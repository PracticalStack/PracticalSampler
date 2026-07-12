# Scriptnode Modulation Infrastructure Reference

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/node_api/helpers/modulation.h`
- `hi_dsp_library/dsp_nodes/ModulationNodes.h`
- `hi_dsp_library/dsp_nodes/EnvelopeNodes.h`
- `hi_dsp_library/dsp_nodes/CableNodeBaseClasses.h`
- `hi_dsp_library/dsp_nodes/RoutingNodes.h`
- `hi_dsp_library/node_api/nodes/processors.h` (wrap::mod)
- `hi_dsp_library/snex_basics/snex_ExternalData.h` (display_buffer_base)

Also consulted: `scriptnode_enrichment/resources/infrastructure/core.md`

---

## 1. Overview: How Modulation Flows in Scriptnode

Scriptnode modulation has two distinct mechanisms:

1. **Modulation sources** (control.* nodes, envelope.* nodes): Nodes that produce
   a value and send it to connected parameter targets. These use `wrap::mod` or
   `control::pimpl::parameter_node_base` to call connected parameters.

2. **Modulation targets** (core.global_mod, core.extra_mod, core.pitch_mod,
   core.matrix_mod): Nodes that pick up downsampled modulation signals from the
   parent HISE sound generator's modulation chains and expose them inside
   scriptnode. These use `modulation::mod_base`.

The two mechanisms are architecturally different. Modulation sources live entirely
within scriptnode's parameter system. Modulation targets bridge the gap between
HISE's traditional modulation system and scriptnode's parameter/signal system.

---

## 2. ModValue: The Modulation Output Primitive

`ModValue` is the fundamental type for communicating modulation output from a node
to the `wrap::mod` wrapper.

```cpp
struct ModValue
{
    void setModValue(double v);              // always set + flag as changed
    bool setModValueIfChanged(double v);     // set only if value differs
    bool getChangedValue(double& d);         // consume: returns true + fills value, clears flag
    double getModValue() const;
    void reset();

    int changed = false;   // flag (int for atomic safety)
    float modValue = 0.0f; // current value (stored as float, not double)
};
```

**Key behaviors:**
- `setModValue()` -- for periodic sources (envelopes, LFOs): always flags changed
- `setModValueIfChanged()` -- for event-driven sources (MIDI, tempo sync): avoids
  redundant parameter updates when value has not actually changed
- `getChangedValue()` -- called by `wrap::mod::checkModValue()`: returns true and
  fills the output if changed, then clears the flag. This is the consumption side.
- The internal storage is `float` even though the API uses `double`. Precision loss
  occurs on storage.

### How a node becomes a modulation source

A node implements:
```cpp
bool handleModulation(double& value);
```

This returns true if a new modulation value should be sent. The simplest pattern
uses ModValue internally:

```cpp
struct my_node {
    ModValue modValue;

    void process(ProcessData<2>& data) {
        // ... compute some value ...
        modValue.setModValue(computedValue);
    }

    bool handleModulation(double& v) {
        return modValue.getChangedValue(v);
    }
};
```

---

## 3. wrap::mod -- The Modulation Source Wrapper

`wrap::mod<ParameterClass, T>` wraps any node T that has `handleModulation()` and
turns it into a modulation source that forwards values to connected parameters.

```
wrap::mod<ParameterClass, T>
  |-- obj: T                    (the wrapped node)
  |-- p: ParameterClass         (parameter connections)
  |-- checkModValue()           (internal: polls handleModulation, calls p)
```

### When checkModValue() is called

The wrapper calls `checkModValue()` after every callback:
- After `process()` -- once per audio block
- After `processFrame()` -- once per sample (performance-sensitive!)
- After `reset()` -- on voice start
- After `handleHiseEvent()` -- on each MIDI event

`checkModValue()` calls `obj.handleModulation(value)`, and if true, calls
`p.call(value)` to forward the value to all connected parameter targets.

### Normalized vs unnormalized modulation

The `isNormalisedModulation()` compile-time trait controls how the modulation
value is interpreted by the receiving parameter:

- **Normalized (default, returns true):** The modulation value is in [0, 1] range.
  The receiving parameter applies its own range mapping (min/max/skew) to convert
  the normalized value to the parameter's actual range.

- **Unnormalized (returns false):** The modulation value is in the target
  parameter's actual range. No range conversion is applied. The C++ code generator
  also skips range scaling for these connections.

Nodes declare this via:
- `static constexpr bool isNormalisedModulation() { return true; }` (default)
- Inheriting from `control::pimpl::no_mod_normalisation` (sets false + registers
  the `UseUnnormalisedModulation` property)

The `no_mod_normalisation` base class also supports marking specific input
parameters as unscaled via `addUnscaledParameter(nodeId, paramName)`.

**OpaqueNode captures this trait** at `create<T>()` time as `isNormalised`.

---

## 4. control::pimpl -- Base Classes for Control Nodes

Control nodes (the `control.*` namespace) are modulation sources that do NOT
process audio. They compute values and forward them to parameter targets.

### parameter_node_base<ParameterType>

The primary base class for control nodes that output to parameters:

```cpp
template <class ParameterType> struct parameter_node_base
{
    parameter_node_base(const Identifier& id) {
        // Registers IsControlNode property -- this bypasses wrap::mod
        CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsControlNode);
    }

    template <int I, class T> void connect(T& t) {
        p.template getParameter<0>().template connect<I>(t);
    }

    ParameterType p;  // the parameter connection(s)
};
```

**Critical distinction:** Control nodes marked with `IsControlNode` bypass
`wrap::mod` entirely. They hold their own `ParameterType p` member and call
`p.call()` directly from their process/event callbacks. The `wrap::mod` wrapper
is only used for nodes that are NOT control nodes (e.g., envelope nodes that
also process audio signal).

### no_processing

Base class for control nodes that are completely outside the signal path:

```cpp
struct no_processing {
    no_processing(const Identifier& id) {
        CustomNodeProperties::addNodeIdManually(id, PropertyIds::OutsideSignalPath);
    }
    // All audio callbacks are empty (SN_EMPTY_*)
    static constexpr bool isNormalisedModulation() { return true; }
};
```

### no_mod_normalisation

Base class for nodes with unnormalized modulation output:

```cpp
struct no_mod_normalisation {
    static constexpr bool isNormalisedModulation() { return false; }

    no_mod_normalisation(const Identifier& nodeId,
                         const StringArray& unscaledInputParameterIds) {
        CustomNodeProperties::addNodeIdManually(nodeId, UseUnnormalisedModulation);
        for (const auto& s : unscaledInputParameterIds)
            CustomNodeProperties::addUnscaledParameter(nodeId, s);
    }
};
```

### templated_mode

Base class for nodes with a compile-time mode selector (template argument):

```cpp
struct templated_mode {
    templated_mode(const Identifier& nodeId, const String& modeNamespace) {
        CustomNodeProperties::addNodeIdManually(nodeId, HasModeTemplateArgument);
        CustomNodeProperties::setModeNamespace(nodeId, modeNamespace);
    }
};
```

The mode namespace maps combobox text entries to C++ class names for compilation.

---

## 5. envelope::pimpl::envelope_base -- Envelope Modulation

Envelopes are a special category of modulation source. They inherit from both
`parameter_node_base<ParameterType>` and `polyphonic_base`.

```cpp
template <typename ParameterType>
struct envelope_base: public control::pimpl::parameter_node_base<ParameterType>,
                      public polyphonic_base
```

### Dual output: CV + Gate

Envelope nodes register two modulation outputs:
```cpp
cppgen::CustomNodeProperties::addModOutput(id, { "CV", "Gate" });
```

- **Output 0 ("CV"):** The continuous envelope value (0..1)
- **Output 1 ("Gate"):** Binary gate signal (0 or 1) -- on when envelope is active

### postProcess() -- The modulation output call

After each process block, envelope nodes call `postProcess()`:

```cpp
template <typename BaseType>
void postProcess(BaseType& t, bool wasActive, double lastValue)
{
    auto thisActive = t.isActive();

    if (thisActive) {
        float mv = (float)t.getModValue();
        FloatSanitizers::sanitizeFloatNumber(mv);
        this->getParameter().template call<0>((double)mv);  // CV output
    }

    if (thisActive != wasActive) {
        this->getParameter().template call<1>((double)(int)thisActive);  // Gate output
        this->getParameter().template call<0>(0.0);  // Reset CV on gate change
    }
}
```

When the active state changes (gate on/off), both outputs are called: Gate gets
the new state, and CV is reset to 0.0.

### Key handling (legato/sustain pedal)

`handleKeyEvent()` implements proper note counting with sustain pedal support:
- Tracks `numKeys` for legato behavior (gate stays high while any key is held)
- Tracks `numSustainedKeys` for CC#64 pedal support
- Returns true only on the first note-on (gate on) or last note-off (gate off)
- Handles all-notes-off for clean state reset

### initialise() -- Dynamic parameter setup

For dynamic parameter types, `initialise()` sets `numParameters` to 2 (CV + Gate)
at runtime.

---

## 6. display_buffer_base -- Ring Buffer for Visualization

`data::display_buffer_base<EnableBuffer>` provides ring buffer writing for
nodes that need visual feedback (modulation plotters, envelope displays, etc.).

```cpp
template <bool EnableBuffer>
struct display_buffer_base : public base,
                             public SimpleRingBuffer::WriterBase
```

The `EnableBuffer` template parameter is a compile-time switch. When false,
all buffer operations compile to nothing (zero overhead).

### Writing to the buffer

Two overloads:
```cpp
void updateBuffer(double v, int numSamples);          // write single value
void updateBuffer(const float* values, int numSamples); // write array
```

Both acquire a `DataReadLock` before writing. Writing only occurs if:
- The ring buffer pointer (`rb`) is not null
- The ring buffer is active (`rb->isActive()`)

### setExternalData() -- Buffer lifecycle

When external data is assigned:
1. Unregisters from previous ring buffer (sets writer to null)
2. Calls `base::setExternalData()` for standard data handling
3. Casts `d.obj` to `SimpleRingBuffer*`
4. Registers as current writer via `rb->setCurrentWriter(this)`
5. Calls `registerPropertyObject(rb)` -- default registers `ModPlotterPropertyObject`
6. If already prepared, calls `prepare()` to resize buffer

### prepare() -- Buffer sizing

```cpp
virtual void prepare(PrepareSpecs ps) {
    lastSpecs = ps;
    if (rb != nullptr) {
        rb->setRingBufferSize(ps.numChannels, numSamples);
        rb->setSamplerate(ps.sampleRate);
    }
}
```

Note: buffer reallocation is skipped when `HI_EXPORT_AS_PROJECT_DLL` is defined
(cannot reallocate across DLL memory boundaries).

### registerPropertyObject() -- Custom display properties

Override this to register a custom property object for specialized displays:
```cpp
virtual void registerPropertyObject(SimpleRingBuffer::Ptr rb) {
    rb->registerPropertyObject<ModPlotter::ModPlotterPropertyObject>();
}
```

Example: `ahdsr_base` overrides this to register `AhdsrRingBufferProperties`
which provides custom path drawing for the AHDSR envelope shape.

### Display downsampling in mod_base

`mod_base` uses `DisplayBufferDownsamplingFactor = 32` to downsample display
writes. In `processFrame()`, it writes once every 64 samples (32 * 2). In
`process()`, it writes `max(2, blockSize / 32)` samples per block.

The `setExternalData()` override in `mod_base` also configures the buffer
length: `BufferLength = 65536 / 32 = 2048` samples.

---

## 7. Modulation Targets: mod_base and the HISE Bridge

`modulation::mod_base` picks up modulation signals from HISE's modulation
system and exposes them inside scriptnode. This is the opposite direction
from `wrap::mod` -- it receives rather than sends.

### Template parameters

```cpp
template <int NV,                    // number of voices
          class IndexClass,          // runtime target indexer
          RuntimeTarget TargetType,  // GlobalModulator or ExternalModulatorChain
          class ConfigClass>         // modulation::config class
struct mod_base;
```

### Inheritance chain

```
mod_base
  : data::display_buffer_base<ConfigClass::shouldEnableDisplayBuffer()>
  : polyphonic_base
  : runtime_target::indexable_target<IndexClass, TargetType, SignalSource>
```

### SignalSource -- The downsampled modulation signal

`modulation::SignalSource` carries the downsampled modulation data from HISE
across DLL boundaries:

```cpp
struct SignalSource {
    double sampleRate_cr;     // control-rate sample rate
    int numSamples_cr;        // control-rate block size
    std::array<pair<Host*, QueryFunction*>, NumMaxModulationSources> modValueFunctions;
};
```

Each slot in `modValueFunctions` is a (Host, QueryFunction) pair. The query
function returns an `EventData` struct containing the modulation signal for
a specific voice/event.

### EventData -- Per-voice modulation state

```cpp
struct EventData {
    SourceType type;              // Disabled, VoiceStart, TimeVariant, Envelope
    const float* signal;          // pointer to modulation signal array
    float constantValue;          // constant value if no dynamic signal
    const int* thisBlockSize;     // current signal array size
    ClearState* resetFlag;        // voice reset state for envelopes
};
```

`getReadPointer()` returns a `VariableStorage`:
- `Types::ID::Void` -- modulation is invalid/disabled
- `Types::ID::Float` -- constant value (no time-varying signal)
- `Types::ID::Block` -- pointer to the dynamic modulation signal array

### Signal rate matching

mod_base detects the relationship between its own sample rate and the
modulation signal's rate:

```cpp
enum class SignalRatio {
    Uninitialised,
    AudioRate,      // node SR / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR == signal SR
    ControlRate,    // node SR == signal SR
    Incompatible    // throws Error::SampleRateMismatch
};
```

- **ControlRate:** The modulation signal can be copied directly (1:1 mapping)
- **AudioRate:** The modulation signal must be expanded/interpolated
  (upsampled by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`)
- **Incompatible:** Throws an error at prepare() time

In frame processing (`processFrame`), audio-rate signals are linearly
interpolated between control-rate samples using `controlRateSubIndex` as
the fractional position.

In block processing (`process`), audio-rate signals are expanded using
`ModBufferExpansion::expand()` which ramps between control-rate values.

---

## 8. TargetMode -- How Modulation Is Applied

`modulation::TargetMode` defines how the modulation signal transforms the
target value. Used by mod_base's `applyModulation()`:

| Mode | Formula (per sample) | Output range | Use case |
|------|---------------------|--------------|----------|
| Gain | `clamp(base - intensity + intensity * mod * base, 0, 1)` | [0, 1] | Traditional HISE intensity scaling |
| Unipolar | `clamp(base + mod * intensity, 0, 1)` | [0, 1] | Additive offset |
| Bipolar | `clamp(base + (2*mod - 1) * intensity, 0, 1)` | [0, 1] | Bipolar offset (mod centered at 0.5) |
| Pitch | (no transformation) | unclamped | Pitch factor passthrough (0.5..2.0) |
| Raw | (no transformation) | unclamped | Pre-calculated signal passthrough |
| Aux | `(1 - intensity + intensity * mod) * base` | unclamped | Intensity-scaled modulation |

**After applying any non-Pitch, non-Raw, non-Aux mode**, the output is clamped
to [0, 1] via `FloatVectorOperations::clip()`.

The `useMidPositionAsZero()` flag (used by Gain mode) shifts the signal by -0.5
before processing and +0.5 after, centering the modulation around 0.5 instead
of 0.0.

### ParameterMode vs TargetMode

These are related but distinct enums:

- `ParameterMode` (stored in connection metadata): ScaleAdd, ScaleOnly, AddOnly,
  Pan, Pitch -- defines how a HISE modulation chain parameter should be modulated
- `TargetMode` (runtime in mod_base): Gain, Unipolar, Bipolar, Pitch, Raw, Aux
  -- defines how the scriptnode target node applies the modulation signal

---

## 9. SourceType -- Categories of HISE Modulation

```cpp
enum class SourceType {
    Disabled,      // No modulation
    VoiceStart,    // Value computed once at note-on (e.g., velocity, random)
    TimeVariant,   // Continuous signal (e.g., LFO, table)
    Envelope        // Continuous with voice lifetime (gate-dependent)
};
```

Stored in `EventData::type`. The `ClearState` enum manages envelope voice
lifecycle:

```cpp
enum class ClearState : int8 {
    Playing,        // Voice is active
    PendingReset1,  // Leave one buffer through (avoids clicks)
    PendingReset2,  // Leave another buffer through (avoids clicks)
    Reset           // Voice is fully released
};
```

The two-stage pending reset prevents glitches at fast release times by allowing
one or two additional buffers to pass through before fully resetting the voice.

---

## 10. Configuration Classes: modulation::config

The `modulation::config` namespace provides policy classes that configure
mod_base behavior at compile time.

### dynamic_internal<EnableBuffer>

Runtime-configurable mode and process signal flag:
```cpp
template <bool EnableBuffer> struct dynamic_internal {
    static constexpr bool shouldEnableDisplayBuffer() { return EnableBuffer; }
    static constexpr bool isNormalisedModulation() { return true; }
    bool shouldProcessSignal() const { return processSignal; }  // runtime
    TargetMode getMode() const { return mode; }                  // runtime

    bool processSignal = false;
    TargetMode mode = TargetMode::Gain;  // default mode
};
```

Aliases: `dynamic` (no buffer), `dynamic_with_display` (with buffer).

### extra_config_internal<EnableBuffer>

For extra modulation chains. Fixed Raw mode, runtime process signal:
```cpp
static constexpr TargetMode getMode() { return TargetMode::Raw; }
```

Aliases: `extra_config` (no buffer), `extra_config_with_display` (with buffer).

### pitch_config_internal<EnableBuffer>

For pitch modulation. Fixed Pitch mode, unnormalized output, mid-position zero:
```cpp
static constexpr bool isNormalisedModulation() { return false; }
static constexpr TargetMode getMode() { return TargetMode::Pitch; }
static constexpr bool useMidPositionAsZero() { return true; }
```

Aliases: `pitch_config` (no buffer), `pitch_config_with_display` (with buffer).

### constant<UseProcess, Mode, EnableDisplayBuffer>

Fully compile-time configuration. Used by the C++ code creator when all
modulation properties are known at compile time:
```cpp
template <bool UseProcess, TargetMode Mode, bool EnableDisplayBuffer=false>
struct constant {
    static constexpr bool shouldProcessSignal() { return UseProcess; }
    static constexpr TargetMode getMode() { return Mode; }
    // All setters are no-ops
};
```

---

## 11. Concrete Modulation Target Nodes

### core::global_mod

Picks up a global modulator signal. Parameters: Index, Value, ProcessSignal,
Mode, Intensity.

- Index: which modulation slot (0-16)
- Value: base value for the modulation formula (default 1.0)
- ProcessSignal: whether to write the modulation signal as audio output
- Mode: Gain / Unipolar / Bipolar
- Intensity: modulation depth (-1 to 1)

### core::extra_mod

Picks up an extra modulation chain signal. Parameters: Index, ProcessSignal.
Fixed Raw mode -- no transformation applied. Simpler than global_mod.

### core::pitch_mod

Picks up pitch modulation. Parameter: ProcessSignal only (no Index -- uses
the fixed PitchIndexer). Unnormalized output (pitch factor values).

The `setExternalData()` override installs a `transformFunction` on the ring
buffer's property object: `ModBufferExpansion::pitchFactorToNormalisedRange`.
This converts pitch factors to [0,1] range for display purposes only.

### core::matrix_mod

Advanced modulation matrix node with source + auxiliary modulation. Uses two
internal `global_mod` instances (source in Raw mode, aux in Aux mode).
Parameters: SourceIndex, Intensity, Mode, Inverted, AuxIndex, AuxIntensity,
ZeroPosition.

Has its own `applyModulation()` with support for inversion and zero-position
offset (center vs left). The `isPlaying()` method checks the source envelope's
block size to determine if the voice is still active.

---

## 12. ProcessSignal Flag

The `ProcessSignal` parameter (present on all mod target nodes) controls whether
the modulation signal is written to the audio output channel:

- **Disabled (0, default):** The node only outputs modulation via `handleModulation()`.
  Audio channels pass through unchanged. The modulation value is computed but only
  a single sample per block is evaluated.

- **Enabled (1):** The node writes the full modulation signal to channel 0 of
  the audio output. This turns the modulation target into a signal source that
  can be processed by downstream audio nodes.

When ProcessSignal is disabled, the smoothing rates for baseValue and intensity
are adjusted: `sampleRate / blockSize` instead of `sampleRate`, since updates
happen once per block rather than per sample.

---

## 13. public_mod -- Exposing Modulation from Nested Networks

`routing::public_mod` creates a modulation signal slot visible to the parent
scope (compiled node or nested network):

```cpp
struct public_mod {
    // OutsideSignalPath + IsPublicMod properties
    // Single parameter "Value" in [0, 1]

    void setParameter<0>(double v) {
        ptr->setModValueIfChanged(v);  // writes to ModValue pointer
    }

    ModValue* ptr = nullptr;  // set during prepare() or connect()
};
```

During `prepare()`, the ModValue pointer is obtained from
`voiceIndex->getTempoSyncer()->publicModValue` -- a somewhat unusual routing
through the tempo syncer infrastructure.

---

## 14. Constants and Limits

| Constant | Value | Meaning |
|----------|-------|---------|
| `NumMaxModulationParameterIndex` | 256 | Max parameter indices in modulation system |
| `NumMaxModulationSources` | `HISE_NUM_MODULATORS_PER_CHAIN` | Max modulation source slots |
| `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` | (preprocessor) | Ratio between audio and control rate |
| `DisplayBufferDownsamplingFactor` | 32 | Display buffer write downsampling |
| Default display buffer length | 2048 | 65536 / 32 |
| Smoothing time | 20ms | For baseValue and intensity sfloat ramps |

---

## 15. Summary: Node Categories and Their Modulation Pattern

| Category | Base class | Output mechanism | Normalized | Display buffer |
|----------|-----------|-----------------|------------|---------------|
| control.* nodes | `parameter_node_base` | Direct `p.call()` | Varies | Varies |
| envelope.* nodes | `envelope_base` (extends `parameter_node_base`) | `postProcess()` -> `p.call()` | true | Via inner node |
| core.global_mod | `mod_base` | `handleModulation()` via ModValue | true | Configurable |
| core.extra_mod | `mod_base` | `handleModulation()` via ModValue | true | Configurable |
| core.pitch_mod | `mod_base` | `handleModulation()` via ModValue | false | Configurable |
| core.matrix_mod | Custom (uses two global_mod internally) | `handleModulation()` via ModValue | true | Always on |
| Any node in wrap::mod | `wrap::mod<P,T>` | `checkModValue()` -> `p.call()` | Varies | N/A |
| routing.public_mod | Standalone | `setModValueIfChanged()` on pointer | true | No |
