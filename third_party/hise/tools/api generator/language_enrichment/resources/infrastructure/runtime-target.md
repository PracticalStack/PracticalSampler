# Runtime Target System and Modulation Bridge Nodes

Distilled from C++ source for the scriptnode node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_tools/hi_tools/runtime_target.h`
- `hi_dsp_library/node_api/helpers/modulation.h`
- `hi_dsp_library/dsp_nodes/ModulationNodes.h`
- `hi_core/hi_dsp/modules/ModulatorChain.h`
- `hi_core/hi_dsp/modules/ModulatorChain.cpp`
- `hi_core/hi_modules/synthesisers/synths/GlobalModulatorContainer.h`
- `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.h`

---

## 1. The runtime_target System

The `runtime_target` system (namespace `hise::runtime_target`) is a
type-erased, DLL-boundary-safe mechanism for connecting scriptnode nodes
to dynamic data sources in the HISE module tree at runtime. It decouples
the DSP library (which lives in the Project DLL) from the HISE engine
(which runs in the IDE or exported plugin).

### Core Architecture

The system has three layers:

1. **Sources** (`source_base`) -- HISE-side objects that produce data.
   Each source has a `RuntimeTarget` type enum and a hash for matching.
2. **Targets** (`target_base` / `typed_target<T>`) -- scriptnode-side
   objects that consume data. Receive typed values via `onValue(T)`.
3. **Connections** (`connection` / `typed_connection<T>`) -- lightweight
   POD structs carrying C-style function pointers for connect/disconnect
   and send-back operations. Connections are created by sources and
   passed to targets for matching.

### RuntimeTarget Enum

```cpp
enum class RuntimeTarget {
    Undefined,
    Macro,
    GlobalCable,           // GlobalRoutingManager::Cable
    NeuralNetwork,
    GlobalModulator,       // GlobalModulatorContainer::RuntimeSource
    ExternalModulatorChain, // ModulatorChain::RuntimeTargetSource
    numRuntimeTargets
};
```

### Indexer Classes

Indexers determine how a target node matches to a source:

| Indexer | Usage | Behavior |
|---|---|---|
| `fix_hash<HashIndex>` | Compiled nodes with fixed connection | `mustBeConnected() = true`, hash is compile-time constant |
| `none` | Unconnected nodes | `mustBeConnected() = true`, hash = -1 (never matches) |
| `dynamic` | Nodes that can change connection at runtime | `mustBeConnected() = false`, hash set via `currentHash` member |

### Connection Lifecycle

1. When a scriptnode network is compiled or instantiated, the
   `GlobalRoutingManager::connectToRuntimeTargets()` iterates all
   cables and calls `OpaqueNode::connectToRuntimeTarget(add, connection)`
   on each node.

2. The OpaqueNode forwards to `indexable_target::connectToRuntimeTarget()`,
   which checks:
   - Does the connection's `RuntimeTarget` type match this target's type?
   - Does the connection's hash match this target's indexer hash?

3. If matched, the target calls the connection's `connectFunction` to
   register itself with the source. The source then calls
   `target->onValue(signal)` to send the current state.

4. On destruction, `indexable_target::disconnect()` calls the
   connection's `disconnectFunction` to unregister.

---

## 2. Modulation Signal Flow

### SignalSource (the message type)

`modulation::SignalSource` is the data object passed from HISE sources
to scriptnode modulation bridge targets via `onValue()`. It contains:

```cpp
struct SignalSource {
    // Array of (Host*, QueryFunction*) pairs -- one per modulation slot
    std::array<std::pair<Host*, EventData::QueryFunction*>,
               NumMaxModulationSources> modValueFunctions;
    double sampleRate_cr;  // control-rate sample rate (host SR / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR)
    int numSamples_cr;     // control-rate block size
};
```

- `NumMaxModulationSources = HISE_NUM_MODULATORS_PER_CHAIN` (default 128)
- `sampleRate_cr` and `numSamples_cr` are downsampled by
  `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` (typically 8)
- Each slot's `QueryFunction` takes a `Host*`, `HiseEvent`, and
  `bool wantsPolyphonicSignal`, returning an `EventData` struct

### EventData (per-voice modulation state)

```cpp
struct EventData {
    SourceType type;           // Disabled, VoiceStart, TimeVariant, Envelope
    const float* signal;       // pointer to modulation signal buffer
    float constantValue;       // constant value (for VoiceStart modulators)
    const int* thisBlockSize;  // current block size (may change per callback)
    ClearState* resetFlag;     // voice reset state (envelopes only)
};
```

The `getReadPointer()` method returns a `VariableStorage`:
- `Types::ID::Void` -- invalid/disabled signal
- `Types::ID::Float` -- constant value (VoiceStart modulators)
- `Types::ID::Block` -- dynamic signal buffer (TimeVariant/Envelope)

### SourceType Enum

| Value | Meaning | Signal Type |
|---|---|---|
| `Disabled` | No modulation | Void |
| `VoiceStart` | Value calculated once at note-on | Float (constant) |
| `TimeVariant` | Continuously changing (LFO, etc.) | Block (buffer) |
| `Envelope` | Per-voice envelope with release tracking | Block (buffer) + resetFlag |

### ClearState (envelope lifecycle)

```cpp
enum class ClearState : int8 {
    Playing,        // Voice is active
    PendingReset1,  // One buffer of tail to avoid clicks
    PendingReset2,  // Second buffer of tail
    Reset           // Voice is fully released
};
```

This state machine prevents audio glitches at fast envelope release
times by allowing two extra buffer cycles after release before marking
the voice as cleared.

---

## 3. HISE-Side Sources

### ModulatorChain::RuntimeTargetSource

Provides a single modulation chain's signal to scriptnode. Used by
`extra_mod` and `pitch_mod` nodes.

- **RuntimeTarget type:** `ExternalModulatorChain`
- **Hash:** Determined by the chain's position in the parent processor:
  - `GainModulation` chain -> hash = `90002` (GainModulation constant)
  - `PitchModulation` chain -> hash = `90001` (PitchModulation constant)
  - Any other chain -> hash = `5000` (CustomOffset constant)
- **Signal type:** Always `SourceType::Envelope`
- **Connection:** Creates a `SignalSource` with a single slot (index 0)
  pointing to itself

The `copyModulationValues()` method is called by the ModulatorChain
during rendering to copy the downsampled modulation signal into the
shared buffer that the connected scriptnode target reads from.

### ModulatorChain::ExtraModulatorRuntimeTargetSource

Provides multiple modulation chains from a hardcoded effect's extra
modulation slots. Used by `extra_mod` nodes.

- **RuntimeTarget type:** `ExternalModulatorChain`
- **Hash:** `5000` (CustomOffset)
- **Signal:** Creates a `SignalSource` with one slot per extra mod chain.
  Each slot maps to a different `RuntimeTargetSource::getEventData`
  function for its respective chain.

The extra mod system supports up to `NumMaxModulationSources` slots.
Each slot can be independently enabled/bypassed based on the
`ParameterProperties` modulation configuration.

### GlobalModulatorContainer::RuntimeSource

Provides access to all modulators in the GlobalModulatorContainer's
gain chain. Used by `global_mod` and `matrix_mod` nodes.

- **RuntimeTarget type:** `GlobalModulator`
- **Hash:** `1` (fixed)
- **Signal:** Creates a `SignalSource` where each slot maps to a
  child modulator in the container's gain chain. Three data types
  are supported:

| Data Class | Modulator Type | Signal Behavior |
|---|---|---|
| `VoiceStartData` | VoiceStartModulator | Returns `Float` constant per note |
| `TimeVariantData` | TimeVariantModulator | Returns `Block` buffer per render call |
| `EnvelopeData` | EnvelopeModulator | Returns `Block` buffer + per-voice tracking |

VoiceStartData stores values indexed by note number (128 slots).
TimeVariantData stores one monophonic buffer.
EnvelopeData stores one buffer per voice (NUM_POLYPHONIC_VOICES channels)
and supports both polyphonic and monophonic read modes.

---

## 4. The mod_base Template

`modulation::mod_base<NV, IndexClass, TargetType, ConfigClass>` is the
core template for all modulation bridge nodes. It inherits:

- `display_buffer_base<ConfigClass::shouldEnableDisplayBuffer()>` --
  optional ring buffer for UI visualization
- `polyphonic_base` -- polyphonic voice management
- `indexable_target<IndexClass, TargetType, SignalSource>` -- runtime
  target connection with type-safe matching

### Template Parameters

| Parameter | Purpose |
|---|---|
| `NV` | Number of voices (1 = mono, typically NUM_POLYPHONIC_VOICES) |
| `IndexClass` | Indexer from `runtime_target::indexers` (determines matching) |
| `TargetType` | `RuntimeTarget` enum value to match against |
| `ConfigClass` | Config class defining mode, normalization, display behavior |

### Key Behaviors

**Signal reception:** When `onValue(SignalSource)` is called by the
HISE source, mod_base stores the signal and calls `checkSignalRatio()`
to determine if the signal is control-rate or audio-rate relative to
the node's own sample rate.

**SignalRatio detection:**
- `ControlRate` -- node sample rate equals signal's control-rate sample rate.
  This happens when the node runs inside a `fix_block` with matching
  block size, or in control-rate containers.
- `AudioRate` -- node sample rate = signal's control rate *
  HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR. This is the normal case --
  the node runs at audio rate and must interpolate the control-rate signal.
- `Incompatible` -- sample rates do not match either ratio. Throws
  `Error::SampleRateMismatch`.

**Voice handling:** On note-on events, `handleHiseEvent()` queries the
SignalSource for voice-specific `EventData` via
`signal.getEventData(slotIndex, event, wantsPolyphonicSignal)`.

**Audio-rate interpolation:** When `SignalRatio::AudioRate`, the
`processFrame()` method linearly interpolates between control-rate
samples using `controlRateSubIndex` as a fractional counter.

### TargetMode (how modulation is applied)

The `applyModulation()` method transforms the raw modulation signal
based on `ConfigClass::getMode()`:

| Mode | Formula | Output Range | Use Case |
|---|---|---|---|
| `Gain` | `baseValue - intensity + intensity * signal * baseValue` | [0, 1] | Classic HISE intensity-scaled gain modulation |
| `Unipolar` | `baseValue + signal * intensity` | [0, 1] clamped | Additive modulation (one direction) |
| `Bipolar` | `baseValue + (2*signal - 1) * intensity` | [0, 1] clamped | Additive modulation (both directions) |
| `Pitch` | pass-through (no transformation) | unbounded | Pitch factor values (0.5..2.0 range) |
| `Raw` | pass-through (no transformation) | unbounded | Pre-calculated signal, no further processing |
| `Aux` | `(1 - intensity + intensity * signal) * baseValue` | varies | Auxiliary modulation (used by matrix_mod) |

When `useMidPositionAsZero()` is true (pitch_config), the Gain mode
subtracts 0.5 before scaling and adds 0.5 after, centering modulation
around the midpoint.

### Parameters (exposed by mod_base)

| Parameter | Setter | Range | Description |
|---|---|---|---|
| `Index` | `setIndex(double)` | [-1, NumMaxModulationSources] | Which modulator slot to read from |
| `Value` | `setValue(double)` | [-1, 1] | Base value (modulation center point) |
| `ProcessSignal` | `setProcessSignal(double)` | 0/1 | Whether to write modulation into channel 0 audio output |
| `Mode` | `setMode(double)` | enum index | TargetMode selection |
| `Intensity` | `setIntensity(double)` | [-1, 1] | Modulation depth/amount |

Not all concrete node types expose all parameters (see section 5).

---

## 5. Concrete Bridge Nodes

### core::global_mod

Bridges a single modulator from the GlobalModulatorContainer into
scriptnode.

- **Base:** `indexable_mod_base` -> `mod_base`
- **RuntimeTarget type:** `GlobalModulator`
- **Default indexer:** `dynamic` (runtime-changeable via Index parameter)
- **Default config:** `dynamic` (runtime Mode/ProcessSignal)
- **Parameters:** Index, Value, ProcessSignal, Mode (Gain/Unipolar/Bipolar), Intensity
- **Mode names in UI:** "Gain", "Unipolar", "Bipolar"
- **Index range:** [0, 16] with step 1
- **CustomNodeProperties:** `IsFixRuntimeTarget`, `NeedsModConfig`,
  `UseUnnormalisedModulation` (if config says not normalized)

### core::extra_mod

Bridges an extra modulation chain from a hardcoded effect into scriptnode.

- **Base:** `indexable_mod_base` -> `mod_base`
- **RuntimeTarget type:** `ExternalModulatorChain`
- **Indexer:** `fix_hash<5000>` (ExtraIndexer = CustomOffset)
- **Config:** `extra_config` -- always `TargetMode::Raw`, normalized
- **Parameters:** Index, ProcessSignal (only 2 parameters)
- **Index range:** [0, 16] with step 1
- **Key difference from global_mod:** No Mode/Value/Intensity parameters.
  The signal is passed through raw because the extra mod system
  handles modulation application externally via `RenderData::handleModulation`.
- **Static assertion:** Template IndexClass must be ExtraIndexer

### core::pitch_mod

Picks up the pitch modulation signal from the parent sound generator's
pitch modulation chain.

- **Base:** `mod_base` directly (NOT through `indexable_mod_base`)
- **RuntimeTarget type:** `ExternalModulatorChain`
- **Indexer:** `fix_hash<90001>` (PitchIndexer = PitchModulation)
- **Config:** `pitch_config` -- `TargetMode::Pitch`, unnormalized,
  `useMidPositionAsZero = true`
- **Parameters:** ProcessSignal only (1 parameter)
- **Key differences:**
  - Does NOT go through `indexable_mod_base` (no NeedsModConfig property)
  - `isNormalisedModulation() = false` -- values are pitch factors, not 0..1
  - `useMidPositionAsZero() = true` -- centers around 0.5
  - Custom `setExternalData` override installs
    `ModBufferExpansion::pitchFactorToNormalisedRange` as a display
    transform function, so the ModPlotter shows sensible visual values
  - Static assertion: Indexer must be PitchIndexer

### core::matrix_mod

A dual-source modulation node that connects to global modulators with
the feature set of the HISE modulation matrix.

- **Does NOT inherit from mod_base.** Instead, it directly inherits:
  - `display_buffer_base<true>` (always has display buffer)
  - `polyphonic_base`
  - `indexable_target<fix_hash<1>, RuntimeTarget::GlobalModulator, SignalSource>`
- **Internally contains two `global_mod` instances:**
  - `sourceMod` -- main modulation source (config: `TargetMode::Raw`)
  - `auxMod` -- auxiliary intensity source (config: `TargetMode::Aux`)
- **Parameters:** SourceIndex, Intensity, Mode (Scale/Unipolar/Bipolar),
  Inverted, AuxIndex, AuxIntensity, ZeroPosition
- **SourceIndex/AuxIndex range:** [-1, NumMaxModulationSources] with step 1,
  default -1 (disconnected)
- **Mode names in UI:** "Scale", "Unipolar", "Bipolar" (note: "Scale"
  instead of "Gain")
- **ZeroPosition:** "Left" (0.0, default) or "Center" (0.5) -- when
  Center, subtracts 0.5 before gain modulation and adds back after
- **Inverted:** Flips the source signal (1 - signal)
- **Envelope voice tracking:** `isPlaying()` checks if the source
  modulator's EventData is an envelope type and whether it has a
  non-zero block size (indicating the voice is still active)
- **Key difference from global_mod:** matrix_mod applies its own
  modulation math with separate source and aux channels. The aux
  channel controls modulation intensity dynamically. The two internal
  global_mod instances use `fix_hash<1>` so they always connect to the
  GlobalModulatorContainer's RuntimeSource (hash = 1).

---

## 6. Config Classes

Config classes customize mod_base behavior at compile time or runtime.
All configs must provide these methods:

```cpp
bool shouldEnableDisplayBuffer();  // Ring buffer for UI visualization
bool isNormalisedModulation();     // Is output 0..1 normalized?
bool shouldProcessSignal();        // Write to audio output channel 0?
TargetMode getMode();              // How to apply modulation
bool useMidPositionAsZero();       // Center around 0.5 in Gain mode?
```

### Config Summary

| Config | Mode | Normalized | Display | Notes |
|---|---|---|---|---|
| `dynamic` | runtime-set | yes | no | Used by global_mod |
| `dynamic_with_display` | runtime-set | yes | yes | global_mod with display |
| `extra_config` | Raw (fixed) | yes | no | Used by extra_mod |
| `extra_config_with_display` | Raw (fixed) | yes | yes | extra_mod with display |
| `pitch_config` | Pitch (fixed) | **no** | no | Used by pitch_mod |
| `pitch_config_with_display` | Pitch (fixed) | **no** | yes | pitch_mod with display |
| `constant<Process, Mode>` | compile-time | yes | optional | C++ compiled nodes |
| matrix_mod `internal_config<M>` | compile-time M | yes | no | Internal to matrix_mod |

The `_with_display` variants differ only in `shouldEnableDisplayBuffer()`.

### extra_config special behavior

`extra_config` has virtual `prepare()` and `initialise()` methods. This
allows a scriptnode wrapper class to override these for validation
(e.g., checking that MIDI processing is enabled on the parent chain).

---

## 7. Hash Matching Summary

How each node type matches to its HISE source:

| Node | RuntimeTarget Type | Hash | Source |
|---|---|---|---|
| `global_mod` | GlobalModulator | dynamic (Index param) | GlobalModulatorContainer |
| `matrix_mod` | GlobalModulator | fix_hash<1> | GlobalModulatorContainer |
| `extra_mod` | ExternalModulatorChain | fix_hash<5000> | ExtraModulatorRuntimeTargetSource |
| `pitch_mod` | ExternalModulatorChain | fix_hash<90001> | ModulatorChain (PitchModulation) |
| (gain chain) | ExternalModulatorChain | fix_hash<90002> | ModulatorChain (GainModulation) |

Note: `global_mod` uses a dynamic indexer so the Index parameter (0-16)
selects which child modulator within the GlobalModulatorContainer to
read from. The index maps to the position in the container's gain chain.

For `extra_mod`, the Index parameter selects which extra modulation
chain to read from within the `SignalSource.modValueFunctions` array.
The hash is fixed at 5000 (CustomOffset) because all extra mod chains
share a single RuntimeTargetSource.

---

## 8. Signal Rate Handling

All HISE modulation signals are downsampled by
`HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` (default 8). The mod_base
template handles upsampling when the scriptnode network runs at audio
rate.

### Block processing (audio rate)

When `SignalRatio::AudioRate`, the `process()` method uses
`ModBufferExpansion::expand()` to upsample the control-rate buffer
to audio rate via linear interpolation (AlignedRamper). If expansion
fails, it falls back to filling with the last known value.

### Frame processing (audio rate)

When `SignalRatio::AudioRate` in `processFrame()`, manual linear
interpolation is performed sample-by-sample using `controlRateSubIndex`
as a sub-sample counter that wraps at HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR.

### Control rate

When `SignalRatio::ControlRate`, the signal buffer is used directly
without interpolation. This occurs when the node is inside a container
that runs at control rate (e.g., `fix_block` with matching block size).

---

## 9. ProcessSignal Flag

The `ProcessSignal` parameter (available on all mod bridge nodes)
controls whether the modulation value is written into channel 0 of
the audio output:

- **Disabled (default):** The node acts as a pure modulation source.
  It updates the `ModValue` for downstream parameter modulation but
  does not modify audio data. The audio signal passes through unchanged.
- **Enabled:** The node writes the modulation value (after applying
  mode/intensity) into `data[0]`, replacing the audio content in
  channel 0. This turns the node into a signal generator, useful for
  routing modulation as an audio signal through the scriptnode graph.

When disabled, the smoothing time for `baseValue` and `intensity` is
adjusted: `sampleRate /= blockSize` (effectively per-block smoothing
instead of per-sample).
