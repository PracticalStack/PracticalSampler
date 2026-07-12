# ValueTreeBuilder: DspNetwork XML to C++ Export

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_snex/snex_cpp_builder/snex_jit_ValueTreeBuilder.h`
- `hi_snex/snex_cpp_builder/snex_jit_ValueTreeBuilder.cpp`

---

## 1. Purpose and Scope

The `ValueTreeBuilder` (namespace `snex::cppgen`) transforms a scriptnode
DspNetwork XML (stored as a JUCE ValueTree) into compilable C++ code. This is
the core of the "Export as C++" pipeline. The output is a single `.h` file
containing nested `using` type aliases that resolve the entire node graph into
a concrete C++ type hierarchy.

The generated code is used in two contexts:
- **CppDynamicLibrary** (project DLL): loaded at runtime by the HISE IDE
- **JitCompiledInstance**: baked directly into an exported plugin binary

Understanding this pipeline is critical for documentation because it determines
which scriptnode features survive export and which are lost.

---

## 2. High-Level Transformation Pipeline

```
DspNetwork XML (ValueTree)
    |
    v
cleanValueTreeIds()     -- sanitize IDs to valid C++ identifiers
    |                      rejects duplicate node IDs and "Type" as parameter name
    v
rebuild()               -- main recursive descent
    |
    +-- parseNode()     -- entry point per node
    |     +-- check UncompileableNode property (throws if found)
    |     +-- add NV template arg if polyphonic
    |     +-- wrap with wrap::no_process if bypassed (non-container)
    |     +-- parseFaustNode() -> parseRuntimeTargetNode() -> parseRoutingNode()
    |     +-- parseOptionalSnexNode() / parseSnexNode() / parseExpressionNode()
    |     +-- parseContainer()
    |     +-- parseMod() -> parseComplexDataNode()
    |
    v
Type alias tree emitted as C++ `using` declarations
    |
    v
RootContainerBuilder   -- generates the initializer class
    |
    +-- metadata struct (data slot counts, encoded parameters, channel count)
    +-- constructor body:
    |     +-- stack variables for all child nodes (this->getT<N>()... chains)
    |     +-- parameter connections (connectT)
    |     +-- modulation connections
    |     +-- send connections
    |     +-- runtime target connections
    |     +-- default parameter values (setParameterT)
    |     +-- setExternalData({}, -1) for complex data init
    +-- static constexpr queries: isPolyphonic(), hasTail(), isSuspendedOnSilence()
    +-- setExternalData() override (routes to child nodes with complex data)
    +-- connectToRuntimeTarget() (if runtime target nodes exist)
```

---

## 3. Node Identity and ID Sanitization

Before code generation, `cleanValueTreeIds()` validates and transforms IDs:

- Node IDs, parameter IDs, and node names are converted to valid C++ identifiers
  via `StringHelpers::makeValidCppName()`
- **Duplicate node IDs are rejected** -- each node must have a unique ID within
  the network
- The parameter name `"Type"` is explicitly forbidden (it conflicts with the HISE
  module XML property)
- Each node ID gets a `_t` suffix in the generated code (e.g., node `"gain1"`
  becomes `gain1_t` in the `impl` namespace)

**Documentation impact:** Users must ensure unique node IDs and avoid reserved
names before exporting. The HISE IDE should catch these, but the error messages
reference C++ identifier rules.

---

## 4. Container Type Resolution

Container factory paths are normalized during export. Many container subtypes
collapse to `container::chain`:

```
container.chain        -> container::chain
container.soft_bypass  -> container::chain + bypass::smoothed<N> wrapper
container.frame*       -> container::chain + wrap::frame<NumChannels> wrapper
container.modchain     -> container::chain + wrap::control_rate wrapper
container.midi         -> container::chain + wrap::event wrapper
container.fix*         -> container::chain + wrap::fix_block<BlockSize> wrapper
container.oversample*  -> container::chain + wrap::oversample<Factor> wrapper
container.offline      -> container::chain + wrap::offline wrapper
container.no_midi      -> container::chain + wrap::no_midi wrapper
container.sidechain*   -> container::chain + wrap::sidechain wrapper (doubles channels)
container.repitch      -> container::chain + wrap::repitch wrapper
container.dynamic_blocksize -> container::chain + wrap::dynamic_blocksize wrapper

container.split        -> container::split (preserved)
container.multi        -> container::multi (preserved)
container.branch       -> container::branch (preserved, fixed parameters)
container.clone        -> wrap::fix_clone* or wrap::clone* (special handling)
```

Key observations:
- Only `split`, `multi`, `branch`, and `clone` retain their container identity
- All other container types become `chain` wrapped with behavior modifiers
- The `fix_block` container requires a **power-of-two block size** (assertion)
- `modchain` forces channel count to 1 for all children
- `multi` divides channel count among children (channels / numChildren)
- `sidechain` doubles the channel count for its children
- Bypassed containers do NOT get `wrap::no_process` -- only bypassed leaf nodes do.
  Instead, bypassed containers skip the special wrapper (frame, midi, etc.)

**Documentation impact:** Container behavior is baked into the C++ type at compile
time. A `container.soft_bypass` with smoothing time 20ms becomes
`bypass::smoothed<20, container::chain<...>>`. The smoothing time cannot change
at runtime in compiled code.

---

## 5. Channel Count Propagation

Channel count flows top-down from the root:

1. Root channel count comes from `CompileChannelAmount` property (default: 2)
2. Each container may modify it for children:
   - `multi`: divides by number of children
   - `modchain`: forces to 1
   - `sidechain`: doubles
3. Non-first children in a chain get no explicit `wrap::fix<N>` -- only the first
   child and all `multi` children get fixed channel wrappers
4. The `ScopedChannelSetter` RAII class manages the current channel count during
   recursive descent

**Documentation impact:** The channel count is resolved at compile time. A network
compiled with 2 channels cannot later process 4 channels. The `wrap::fix<N>` wrapper
enforces this statically.

---

## 6. Parameter Connection Types

Parameters are resolved to typed C++ templates based on their connection
characteristics:

| Connection Type | C++ Template | Condition |
|---|---|---|
| No connection | `parameter::empty` | No targets |
| Identity range, single target | `parameter::plain<Node, Index>` | Range is identity (0..1 -> 0..1) |
| Inverted identity | `parameter::inverted<Node, Index>` | Identity range but inverted |
| Scaled range | `parameter::from0To1<Node, Index, Range>` | Non-identity target range |
| Inverted scaled | `parameter::from0To1_inv<Node, Index, Range>` | Inverted + non-identity |
| Bypass target | `parameter::bypass<Node, Range?>` | Target is `Bypassed` param |
| Expression | `parameter::expression<Node, Index, Expr>` | Has expression code |
| Multiple targets | `parameter::chain<InputRange, P1, P2, ...>` | >1 connection from same source |

Ranges are emitted as `DECLARE_PARAMETER_RANGE` / `DECLARE_PARAMETER_RANGE_SKEW` /
`DECLARE_PARAMETER_RANGE_STEP` macros (with `_INV` suffix for inverted). Identical
ranges are pooled -- shared across parameters that use the same range.

**Documentation impact:** Parameter ranges are baked at compile time. The skew,
step, min, max, and inversion of every connection become template constants. Users
cannot change parameter ranges in compiled code.

---

## 7. Modulation Connections

Modulation sources (nodes with `ModulationTargets` children) get wrapped:

- **Control nodes** (IsControlNode property): modulation parameter is the first
  template argument of the node type itself (no `wrap::mod` needed)
- **Signal-processing nodes** with mod output: wrapped in `wrap::mod<Parameter, Node>`

Clone cables use `parameter::cloned` and `parameter::clonechain` for distributing
values across clone instances.

Switch targets (multi-output modulation) use `parameter::list` with indexed
`SwitchTarget` connections.

---

## 8. Complex Data Handling

Nodes with complex data (tables, slider packs, audio files, filters, display
buffers) get wrapped in `wrap::data<Node, DataSpec>`:

| Scenario | DataSpec | Details |
|---|---|---|
| Single external slot | `data::external::table<Index>` | Index = slot number in parent |
| Single embedded data | `data::embedded::table<DataStruct>` | Data baked as float array |
| Embedded audio file | `data::embedded::audiofile<DataStruct>` | Multichannel with metadata |
| Multiple data types | `data::matrix<MatrixStruct>` | Index mapping matrix |
| Display buffer only, no external | `wrap::no_data<Node>` | Buffer disabled in compiled code |
| Display buffer, external | `data::external::displaybuffer<Index>` | Buffer enabled |

Embedded data is stored as:
- **Tables**: 512-float lookup table (SAMPLE_LOOKUP_TABLE_SIZE)
- **SliderPacks**: variable-length float array
- **AudioFiles**: channel data as reinterpret_cast float arrays, with metadata
  (sample count, sample rate, channel count)

The `data::matrix` handles complex routing where a single node uses multiple data
types or non-sequential slot indices. Embedded data slots use index >= 1000 in the
matrix.

**Documentation impact:** External data connections (which slot feeds which node)
are fixed at compile time. Embedded data is baked into the binary. Audio files
referenced by path must be loadable at export time or export fails.

---

## 9. Special Node Handling

### Faust nodes (`core.faust`)
Replaced with `project::ClassName` referencing the Faust-generated C++ class.
The Faust class ID is extracted from the node's `ClassId` property. Always gets
`NV` polyphonic template argument.

### SNEX nodes (`core.snex_node` and others)
- Unwrapped SNEX: node type becomes the SNEX class directly
- Wrapped SNEX: SNEX class becomes a template argument of the wrapper node type
- Code is injected via a `CodeProvider` interface

### Expression nodes (`math.*expr`, `control.*expr`)
Custom expressions are emitted as inline `static float/double op()` functions
inside a `custom` namespace struct. The expression code from the property becomes
the return statement body.

### Routing nodes (`routing::matrix`)
The routing matrix is decoded from base64-encoded embedded data and emitted as a
`routing::static_matrix<NumChannels, MatrixClass, HasSend>` with compile-time
channel and send arrays.

### Runtime target nodes
Nodes with `IsFixRuntimeTarget` or `IsDynamicRuntimeTarget` get an indexer
template argument:
- Fixed: `runtime_target::indexers::fix_hash<HashCode>` -- hash computed from
  connection property or special constants (PitchModulation, CustomOffset)
- Dynamic: `runtime_target::indexers::dynamic`

Special cases for `math::neural`: additional `HpfFrequency` template argument
mapped from property strings ("Off", "1 Hz", "5 Hz").

Nodes with `NeedsModConfig` get a `modulation::config::constant` or
`modulation::config::dynamic` template argument encoding whether the node
processes signal and its modulation target mode (Gain/Unipolar/Bipolar/Raw).

---

## 10. Polyphony Resolution

Polyphony is resolved at compile time via the `NV` template parameter:

- Nodes with `IsPolyphonic` property get `NV` as a template integer argument
- The root container class is templated on `NV` if any child is polyphonic
- `static constexpr bool isPolyphonic() { return NV > 1; }` is emitted
- The `BETTER_TEMPLATE_FORWARDING` flag (always 1) means NV propagates through
  template forwarding rather than explicit addition at each level

**Documentation impact:** A monophonic network (`NV=1`) and polyphonic network
use the same source code -- the template parameter controls whether `PolyData`
stores one value or NUM_POLYPHONIC_VOICES values.

---

## 11. What is Preserved vs Lost in C++ Export

### Preserved (baked as compile-time constants)
- Complete node graph topology
- Parameter ranges (min, max, skew, step, inversion)
- Parameter connections and modulation routing
- Default parameter values
- Complex data (tables, slider packs, embedded audio)
- Routing matrices
- Polyphony support (as NV template)
- Channel count
- hasTail / suspendOnSilence flags
- Container type and wrapper chain
- Expression code
- Faust/SNEX custom DSP code
- Send connections
- Clone configuration (static or dynamic NumClones)
- Runtime target hash codes

### Lost or changed
- **Dynamic reconfiguration**: no adding/removing nodes at runtime
- **Parameter range changes**: ranges are compile-time constants
- **Container type switching**: a chain cannot become a split
- **Block size flexibility** (fix_block): block size is a template constant
- **Oversampling factor**: baked as template constant
- **Soft bypass smoothing time**: baked as template constant
- **Routing matrix**: channel mapping is a static constexpr array
- **Display buffers for embedded-only nodes**: wrapped in `wrap::no_data` (disabled)
- **Node comments**: preserved only as C++ comments, no runtime effect
- **Unused modulation outputs**: control nodes without targets get `parameter::empty`
  with a `jassertfalse` (debug assertion -- likely a network error)

---

## 12. Compilability Constraints

Nodes can be marked as uncompilable via `CustomNodeProperties`:

- `UncompileableNode`: export throws immediately with descriptive error
- `AllowCompilation`: positive flag (absence does not block, but related UI
  may hide the export option)

### Export validation errors
The builder throws `Error` exceptions (with ValueTree context) for:
- Duplicate node IDs
- "Type" as parameter name
- Uncompilable nodes in the network
- Missing SNEX/Faust class IDs
- Dangling parameter connections (target node not found)
- Recursive parameter connections (modulation output targeting parent container)
- Clone sanity failures:
  - NumClones mismatch between container and cable
  - Range mismatch between clone container and cable
  - NumClones must be first target in root parameter connection
- Invalid fix_block size (not power of two, or zero)
- Missing audio file references for embedded data

---

## 13. Generated Code Structure

For a network named `MyNetwork` with CppDynamicLibrary format:

```cpp
#pragma once

// project:: includes for custom nodes
// Convenience macros: getT, connectT, getParameterT, setParameterT, setParameterWT

using namespace scriptnode;
using namespace snex;
using namespace snex::Types;

namespace MyNetwork_impl {

// ---- Node & Parameter type declarations ----

// Range macros
DECLARE_PARAMETER_RANGE(paramName_Range, 0.0, 1.0);

// Parameter types (using aliases)
using paramName = parameter::from0To1<nodeA_t, 0, paramName_Range>;

// Node types (nested using aliases, innermost = leaf nodes)
using gain1_t = wrap::fix<2, core::gain>;
using chain1_t = container::chain<parameter::plain<gain1_t, 0>, gain1_t>;

// Root node type alias
using instance_ = container::chain<paramList, chain1_t, ...>;

// ---- Root node initialiser class ----
template <int NV> struct instance : public instance_<NV> {
    struct metadata {
        static const int NumTables = 0;
        static const int NumSliderPacks = 0;
        static const int NumAudioFiles = 0;
        static const int NumFilters = 0;
        static const int NumDisplayBuffers = 0;
        SNEX_METADATA_ID(MyNetwork);
        SNEX_METADATA_NUM_CHANNELS(2);
        SNEX_METADATA_ENCODED_PARAMETERS(N) ...
    };

    instance() {
        // Stack variables for node access
        auto& gain1 = this->getT(0).getT(0);
        // Parameter connections
        auto& p = this->getParameterT(0);
        p.connectT(0, gain1);
        // Default values
        gain1.setParameterT(0, 0.5);
    }

    static constexpr bool isPolyphonic() { return NV > 1; }
    static constexpr bool hasTail() { return false; }
    static constexpr bool isSuspendedOnSilence() { return false; }
};
}

#undef getT
#undef connectT
// ...

namespace project {
    using MyNetwork = wrap::node<MyNetwork_impl::instance>;
}
```

The final public type is always `wrap::node<impl::instance>` in the `project`
namespace.
