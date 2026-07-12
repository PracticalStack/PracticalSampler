# Scriptnode Container Infrastructure Reference

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/node_api/nodes/Containers.h` -- container_base template
- `hi_dsp_library/node_api/nodes/Container_Chain.h` -- chain, branch containers
- `hi_dsp_library/node_api/nodes/Container_Split.h` -- split container
- `hi_dsp_library/node_api/nodes/Container_Multi.h` -- multi container
- `hi_dsp_library/node_api/nodes/container_base.h` -- (empty, placeholder)
- `hi_dsp_library/node_api/helpers/parameter.h` -- tuple iterator macros
- `hi_scripting/scripting/scriptnode/nodes/NodeContainer.h` -- interpreted NodeContainer
- `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h` -- all dynamic container types
- `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.cpp` -- process implementations

Also consulted:
- `scriptnode_enrichment/resources/infrastructure/core.md` -- core types (ProcessData, PrepareSpecs, etc.)

---

## 1. Architecture Overview: Compiled vs Interpreted Containers

Scriptnode containers exist in two forms:

### Compiled (C++ template) containers

Located in `hi_dsp_library/node_api/nodes/`. These are variadic templates that
store child nodes as `std::tuple<Processors...>`. All iteration is compile-time
via `std::index_sequence` fold expressions. Used in hardcoded/exported C++ nodes.

- `container::chain<ParameterClass, Processors...>` -- serial processing
- `container::split<ParameterClass, Processors...>` -- parallel with summing
- `container::multi<ParameterClass, Processors...>` -- parallel with channel splitting
- `container::branch<Unused, Processors...>` -- index-selected single child

### Interpreted (dynamic) containers

Located in `hi_scripting/scripting/scriptnode/nodes/`. These use a runtime
`NodeBase::List` (ReferenceCountedArray) and iterate with for-loops. Used in
the scriptnode IDE for live editing.

- `ChainNode` (factory ID: `chain`) -- serial, wraps `bypass::simple<DynamicSerialProcessor>`
- `SplitNode` (factory ID: `split`) -- parallel with summing
- `MultiChannelNode` (factory ID: `multi`) -- parallel with channel splitting
- `BranchNode` (factory ID: `branch`) -- index-selected single child
- `ModulationChainNode` (factory ID: `modchain`) -- serial, 1 channel, control rate
- `MidiChainNode` (factory ID: `midichain`) -- serial with MIDI event forwarding
- `NoMidiChainNode` (factory ID: `no_midi`) -- serial, blocks MIDI events
- `SoftBypassNode` (factory ID: `soft_bypass`) -- serial with smoothed bypass
- `CloneNode` (factory ID: `clone`) -- array of equal nodes, dynamic resizing
- `OversampleNode<N>` (factory ID: `oversample`/`oversampleNx`) -- serial with oversampling
- `FixedBlockNode<B>` (factory ID: `fixN_block`) -- serial with fixed block size
- `FixedBlockXNode` (factory ID: `fix_blockx`) -- serial with dynamic block size property
- `DynamicBlockSizeNode` (factory ID: `dynamic_blocksize`) -- serial with parameter-controlled block size
- `SingleSampleBlock<N>` (factory ID: `frameN_block`) -- serial, frame-based processing
- `SingleSampleBlockX` (factory ID: `framex_block`) -- serial, frame-based, dynamic channels
- `RepitchNode` (factory ID: `repitch`) -- serial with resampling
- `SidechainNode` (factory ID: `sidechain`) -- serial, doubles channel count
- `OfflineChainNode` (factory ID: `offline`) -- serial, offline processing

All dynamic containers are registered by `NodeContainerFactory` with factory
namespace `"container"`. The factory ID determines the node path:
`container.chain`, `container.split`, etc.

---

## 2. container_base: The Compiled Container Foundation

```cpp
template <class ParameterClass, typename... Processors>
struct container_base
```

### Template parameters

- `ParameterClass` -- a parameter tuple type that holds compile-time parameter
  connections. For containers with no parameters, use `parameter::empty`.
- `Processors...` -- the child node types (each typically a wrapped node)

### Storage

Child nodes are stored as a `std::tuple<Processors...> elements`. Access is via:
- `get<N>()` -- returns reference to the Nth child's inner object (calls `.getObject()`)
- `getParameter<N>()` -- returns the Nth parameter from the ParameterClass

### Common callbacks (defined in container_base)

| Callback | Implementation |
|---|---|
| `initialise(ObjectWithValueTree*)` | Iterates all children, calls `initialise()` on each |
| `reset()` | Iterates all children, calls `reset()` on each |
| `prepare(PrepareSpecs)` | Declared as tuple_iterator but NOT called -- subclasses override |
| `handleHiseEvent(HiseEvent&)` | Declared as tuple_iterator but NOT called -- subclasses override |

Note: `prepare()` and `handleHiseEvent()` are declared as protected tuple
iterators in container_base, but each container subclass calls them explicitly
via `call_tuple_iterator1()` in its own `prepare()` / `handleHiseEvent()`.
This gives subclasses control over the order and context of these calls.

### Compile-time traits

- `isModulationSource = false` -- containers are not modulation sources
- `getFixChannelAmount()` -- returns `NumChannels` of the first child element
- `isPolyphonic()` -- delegates to `get<0>().isPolyphonic()`

---

## 3. Tuple Iteration Mechanism

The macro system in `parameter.h` generates compile-time iteration functions
that call a method on every element in the `std::tuple`.

### How it works

The `tuple_iterator1(name, type, arg)` macro expands to:

```cpp
template <std::size_t ...Ns>
void name_each(type arg, std::index_sequence<Ns...>)
{
    using swallow = int[];
    (void)swallow { 1, (std::get<Ns>(this->elements).name(arg), void(), int{})... };
}
```

This uses a C++17 fold expression over a pack expansion. For each index `Ns`,
it calls `std::get<Ns>(elements).name(arg)`. The `swallow` array trick
guarantees left-to-right evaluation order.

### Operator variant: tuple_iterator_op

For `process()` and `processFrame()`, containers use a functor pattern:

```cpp
tuple_iterator_op(process, BlockProcessor);
```

This expands to call `data(std::get<Ns>(elements))` -- the functor's
`operator()` is invoked with each child node. This allows containers to
customize how each child is processed (e.g., split copies the buffer,
multi slices channels).

### Calling the iterators

From the container's public methods:
```cpp
call_tuple_iterator1(prepare, ps);   // expands to: this->prepare_each(ps, Type::getIndexSequence());
call_tuple_iterator0(reset);         // expands to: this->reset_each(Type::getIndexSequence());
```

---

## 4. Chain Processing (Serial Signal Flow)

### Compiled: `container::chain`

```
Input -> P1 -> P2 -> ... -> Pn -> Output
```

All children share the same channel count: `NumChannels = first child's NumChannels`.

**Block processing (`process`):**
Uses `chainprocessor::Block<ProcessDataType>` functor. The functor simply calls
`obj.process(d)` on each child in sequence, passing the same `ProcessDataType`
reference. Each child modifies the buffer in-place, and the next child sees
the modified result.

**Frame processing (`processFrame`):**
Uses `chainprocessor::Frame<FrameType>` functor. Same pattern: calls
`obj.processFrame(d)` on each child with the same frame reference.

**Event handling:**
All children receive the same `HiseEvent&` reference via `call_tuple_iterator1`.

**Prepare:**
All children receive identical `PrepareSpecs`.

### Interpreted: `ChainNode`

Wraps `DynamicSerialProcessor` in `bypass::simple<>`. The `DynamicSerialProcessor`
iterates `parent->getNodeList()` with a for-each loop:

```cpp
for (auto n : parent->getNodeList())
    n->process(data.as<ProcessDataDyn>());
```

The `bypass::simple` wrapper adds bypass support: when bypassed, the chain
is skipped entirely.

---

## 5. Split Processing (Parallel with Summing)

### Signal flow

```
Input ---+---> P1 (processes original) ---+
         |                                |
         +---> P2 (processes copy) -------+--> Sum -> Output
         |                                |
         +---> Pn (processes copy) -------+
```

### Compiled: `container::split`

Channel count: same as first child (`NumChannels = first child's NumChannels`).

**Key buffers:**
- `originalBuffer` (heap<float>) -- stores a copy of the input before processing
- `workBuffer` (heap<float>) -- scratch space for non-first children

**prepare():**
Calls prepare on all children, then allocates `originalBuffer` and `workBuffer`
(only when N > 1 children exist).

**Block processing (`process`):**
Uses `splitprocessor::Block` functor with a `channelCounter`:
1. On entry, copies the input to `originalBuffer`
2. First child (counter == 0): processes the original data in-place
3. Subsequent children: copies `originalBuffer` to `workBuffer`, processes
   `workBuffer`, then ADDS the result back to the main data buffer using
   `FloatVectorOperations::add()`
4. Single-element optimization: if only one child, skips the copy entirely

**Frame processing:**
Same pattern but with stack-allocated `FrameType` copies instead of heap buffers.

**Event handling:**
The interpreted `SplitNode` creates a copy of the HiseEvent for each child:
```cpp
HiseEvent copy(e);
n->handleHiseEvent(e);
```
Note: The compiled version in container_base also passes the reference, but
the split/multi nodes copy the event to prevent children from modifying it
for subsequent siblings.

### Interpreted: `SplitNode`

Extends `ParallelNode`. Allocates `original` and `workBuffer` as `heap<float>`.
Skips bypassed children in the loop. Frame processing dispatches to
`processFrameInternal<C>()` which handles mono (C=1) and stereo (C=2) cases.

---

## 6. Multi Processing (Parallel with Channel Splitting)

### Signal flow

```
Input (ch0, ch1, ch2, ch3)
  |
  +-- P1 gets (ch0, ch1)    [NumChannels = 2]
  +-- P2 gets (ch2, ch3)    [NumChannels = 2]
  |
Output (ch0, ch1, ch2, ch3)
```

### Compiled: `container::multi`

**Critical difference from chain/split:** The total channel count is the SUM
of all children's channel counts, not the first child's count:

```cpp
constexpr static int NumChannels = Helpers::getSummedChannels<Processors...>();
```

**Block processing:**
Uses `multiprocessor::Block` functor with a `channelIndex` counter:
1. For each child, reads its `T::NumChannels` at compile time
2. Creates a `ProcessData<NumChannelsThisTime>` pointing at
   `d.getRawDataPointers() + channelIndex`
3. Copies non-audio data (events) from the parent
4. Calls `obj.process(thisData)`
5. Advances `channelIndex += NumChannelsThisTime`

Each child processes its own channel slice in-place. No copying or summing.

**Frame processing:**
Uses `reinterpret_cast` to create a `span<float, NumChannelsThisTime>` pointing
into the parent frame at the correct channel offset.

**Event handling:**
Creates a copy of the HiseEvent for each child (same as split).

### Interpreted: `MultiChannelNode`

Uses dynamic channel routing with `channelRanges[NUM_MAX_CHANNELS]` array.
The `channelLayoutChanged()` method recalculates ranges when children are
added/removed. Each child's channel range is determined by its
`getCurrentChannelAmount()`.

The dynamic version guards against exceeding available channels:
```cpp
if (endChannel <= d.getNumChannels())
```

---

## 7. Branch Processing (Index-Selected Single Child)

### Signal flow

```
Input --> switch(currentIndex) --> P[index] --> Output
```

Only one child processes at a time. All others are idle.

### Compiled: `container::branch`

Uses `parameter::branch_index<N>` as its ParameterClass, which stores a
`uint8 currentIndex`. The parameter is automatically the first (and only)
macro parameter.

**Processing uses a switch/case macro:**
```cpp
#define CASE(idx, function, arg) case idx: this->template get<jmin(NumElements-1, idx)>().function(arg); break
#define CASE_16(function, arg) CASE(0, ...); CASE(1, ...); ... CASE(16, ...);
```

Maximum 16 children (enforced by `static_assert` in `prepare()`).

The `jmin(NumElements-1, idx)` ensures that cases beyond the actual number of
children map to the last child, avoiding out-of-bounds template instantiation.

**prepare() and reset():**
All children are prepared and reset, not just the active one. This ensures
any child can be switched to at any time without audible artifacts from
uninitialized state.

**handleHiseEvent():**
Only the currently selected child receives events.

### Interpreted: `BranchNode`

Stores `int currentIndex`. The `setIndex()` parameter callback updates it.
`updateIndexLimit()` listens for child add/remove and adjusts the parameter's
max value. Default range is 0-10 with step 1.

---

## 8. Specialized Serial Containers

These are all `SerialNode` subclasses that wrap `DynamicSerialProcessor` in
a specific wrapper template. They modify the processing context before
delegating to the serial child chain.

### ModulationChainNode (`modchain`)

- Forces `numChannels = 1` in prepare
- Wraps in `wrap::fix<1, wrap::control_rate<...>>` -- processes at control rate
- Does NOT modify sample rate (asserts original sample rate)
- Used for modulation source chains that output control signals

### MidiChainNode (`midichain`)

- Wraps in `wrap::event<...>` -- ensures MIDI events are forwarded
- No audio processing modifications

### NoMidiChainNode (`no_midi`)

- Wraps in `wrap::no_midi<...>` -- blocks MIDI event forwarding to children
- Useful for preventing note-on/off from reaching specific processing chains

### SoftBypassNode (`soft_bypass`)

- Wraps in `bypass::smoothed<-1, ...>` -- crossfades between bypassed/active
- Has a `smoothingTime` NodeProperty (integer, in ms)
- Avoids clicks when toggling bypass

### OversampleNode (`oversample` / `oversampleNx`)

- Wraps in `wrap::oversample<Factor, ...>`
- Template parameter: OversampleFactor (-1 = dynamic, or fixed 2/4/8/16)
- Parameters: OversamplingFactor (exponent 0-4), FilterType (Polyphase or FIR)
- `getSampleRateForChildNodes()` returns upsampled rate
- `getBlockSizeForChildNodes()` returns upsampled block size
- `processFrame()` asserts false -- frame processing not supported with oversampling

### FixedBlockNode (`fixN_block`)

- Wraps in `wrap::fix_block<BlockSize, ...>`
- Template parameter: B (fixed block size: 8, 16, 32, 64, etc.)
- Splits incoming audio into fixed-size chunks
- `getBlockSizeForChildNodes()` returns B (or original if bypassed or frame mode)
- When bypassed, reverts to original block size

### FixedBlockXNode (`fix_blockx`)

- Wraps in `wrap::fix_blockx<...>` with a `DynamicBlockProperty`
- Block size is a string NodeProperty (valid values: 8, 16, 32, 64, 128, 256, 512)
- Uses a switch statement to dispatch to the correct `fix_block<N>` template
- Must be a power of two and >= 8; defaults to 64 if invalid

### DynamicBlockSizeNode (`dynamic_blocksize`)

- Wraps in `wrap::dynamic_blocksize<...>`
- Block size is a parameter (not a property), allowing modulation
- Has `hasFixedParameters() = true`

### SingleSampleBlock (`frameN_block`)

- Wraps in `wrap::frame<NumChannels, ...>`
- Forces blockSize=1 for children (frame-by-frame processing)
- Fixed channel count at compile time (template parameter)
- Handles channel mismatch with a leftover buffer for extra channels
- When bypassed, reverts to block processing (passes through)

### SingleSampleBlockX (`framex_block`)

- Wraps in `wrap::frame_x<...>` -- dynamic channel count version
- Same concept as SingleSampleBlock but channel count is runtime

### RepitchNode (`repitch`)

- Wraps in `wrap::repitch<..., wrap::interpolators::dynamic>`
- Parameters: RepitchFactor, Interpolation
- Resamples audio, processes at different effective sample rate
- `processFrame()` is a no-op (block processing only)

### SidechainNode (`sidechain`)

- Wraps in `wrap::sidechain<...>`
- Doubles the channel count by creating empty duplicate channels
- `getNumChannelsToDisplay()` returns `numChannels * 2`
- Used for effects that need a sidechain input

### OfflineChainNode (`offline`)

- Wraps in `wrap::offline<...>`
- For offline/non-realtime processing

### CloneNode (`clone`)

- Uses `wrap::clone_base<DynamicCloneData, CloneProcessType::Dynamic>`
- Parameters: NumClones, SplitSignal
- Maintains an array of identical child nodes
- Syncs property changes, parameter ranges, and modulation connections across clones
- `DynamicCloneData::Iterator` controls how many clones are active

---

## 9. NodeContainer: The Interpreted Container Base

All interpreted container nodes inherit from `NodeContainer` (which itself
is mixed into `NodeBase` via `SerialNode` or `ParallelNode`).

### Key members

- `NodeBase::List nodes` -- the child node list (ReferenceCountedArray)
- `originalSampleRate`, `originalBlockSize` -- stored during prepare
- `PolyHandler* lastVoiceIndex` -- voice handler from last prepare

### Prepare flow

1. `prepareContainer(ps)` -- stores original sample rate and block size,
   applies container-specific modifications
2. `prepareNodes(ps)` -- iterates children, calls `prepare()` on each
3. Virtual hooks: `getBlockSizeForChildNodes()`, `getSampleRateForChildNodes()`
   allow containers to modify specs before passing to children

### Child node management

- `nodeListener` -- ValueTree child listener; fires `nodeAddedOrRemoved()`
  when children are added/removed from the Nodes child tree
- `parameterListener` -- tracks macro parameter additions/removals
- `channelListener` -- recursive property listener for channel layout changes
- `forEachNode()` -- recursive iteration over all children and sub-children

### SerialNode vs ParallelNode

- **SerialNode** -- base for all serial containers. Contains
  `DynamicSerialProcessor` which iterates `getNodeList()` sequentially.
  Has `isVertical` property for UI layout.
- **ParallelNode** -- base for split, multi, and branch. No special
  processing logic of its own; subclasses implement the routing.

### MacroParameter

Container-level parameters that forward to child node parameters.
`MacroParameter` extends `NodeBase::Parameter` with `ConnectionSourceManager`
for managing parameter-to-parameter connections.

---

## 10. Channel Count Rules

| Container | NumChannels (compiled) | NumChannels (interpreted) |
|---|---|---|
| chain | First child's NumChannels | Dynamic (from network) |
| split | First child's NumChannels | Dynamic (from network) |
| multi | Sum of all children's NumChannels | Sum of children's getCurrentChannelAmount() |
| branch | First child's NumChannels | Dynamic (from network) |

For compiled containers, the channel count is a compile-time constant baked
into the ProcessData template parameter. For interpreted containers, the
channel count is determined at runtime by the DspNetwork and can change when
nodes are added or removed.

The `multi` container is unique: it requires enough total channels to cover
all children. If the host provides fewer channels than the sum, the
interpreted version silently skips children that exceed the available channels.

---

## 11. How Containers Interact with Wrappers

Most specialized containers wrap `DynamicSerialProcessor` (or a compiled
`Processors...` tuple) in one or more wrapper templates. The wrapper modifies
the processing context:

```
OversampleNode:  wrap::oversample<Factor, DynamicSerialProcessor>
FixedBlockNode:  wrap::fix_block<BlockSize, DynamicSerialProcessor>
SoftBypassNode:  bypass::smoothed<-1, DynamicSerialProcessor>
ChainNode:       bypass::simple<DynamicSerialProcessor>
ModChainNode:    wrap::fix<1, wrap::control_rate<DynamicSerialProcessor>>
MidiChainNode:   wrap::event<DynamicSerialProcessor>
NoMidiChainNode: wrap::no_midi<DynamicSerialProcessor>
SidechainNode:   wrap::sidechain<DynamicSerialProcessor>
FrameNode:       wrap::frame<NumCh, DynamicSerialProcessor>
RepitchNode:     wrap::repitch<DynamicSerialProcessor, interpolator>
CloneNode:       wrap::clone_base<DynamicCloneData, Dynamic>
```

Wrappers can be nested. For example, `modchain` uses
`wrap::fix<1, wrap::control_rate<...>>` -- first fixes channel count to 1,
then applies control-rate processing.

In compiled/exported code, the same wrappers are applied to the entire
container template rather than to DynamicSerialProcessor. For example:
```cpp
wrap::fix_block<64, container::chain<parameter::empty, node1, node2>>
```

---

## 12. Event Handling Patterns

| Container | Event routing |
|---|---|
| chain | All children receive the same event reference (in order) |
| split | Each child gets a copy of the event (compiled: same ref, interpreted: copy) |
| multi | Each child gets a copy of the event |
| branch | Only the currently selected child receives the event |
| midichain | Forces event processing ON for children |
| no_midi | Blocks events from reaching children |

**Note on split event handling (interpreted):** The SplitNode creates a
`HiseEvent copy(e)` but then passes `e` (not `copy`) to
`n->handleHiseEvent(e)`. The copy is unused. This means the first child can
modify the event and subsequent children see the modified version -- same
behavior as chain. This differs from the compiled split, where the same
reference is passed but children typically don't modify events.

---

## 13. Bypass Behavior

- `ChainNode` uses `bypass::simple` -- hard bypass, no crossfade
- `SoftBypassNode` uses `bypass::smoothed` -- crossfade with configurable time
- `OversampleNode`, `FixedBlockNode`, `SingleSampleBlock` -- override
  `setBypassed()` to re-prepare with original specs when bypass changes
  (since block size / sample rate changes affect child preparation)
- `SplitNode` -- checks `isBypassed()` at process entry, returns immediately
- `BranchNode` -- checks `isBypassed()` at process entry, returns immediately
- Bypassed children in a split are individually skipped in the loop

---

## 14. Summary Table: Container Characteristics

| Container | Signal Flow | Channel Model | Children Processed | Buffers Required | Max Children |
|---|---|---|---|---|---|
| chain | Serial | Shared (first child) | All, sequentially | None (in-place) | Unlimited |
| split | Parallel+Sum | Shared (first child) | All, independently | original + work | Unlimited |
| multi | Parallel+Slice | Sum of children | All, own channels | None (pointer offset) | Limited by total channels |
| branch | Selective | Shared (first child) | One (by index) | None | 16 (compiled) |
| clone | Parallel | Shared or split | NumClones subset | Depends on split mode | Unlimited |
