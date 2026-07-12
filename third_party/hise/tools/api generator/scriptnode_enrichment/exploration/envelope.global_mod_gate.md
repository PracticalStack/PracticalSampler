# envelope.global_mod_gate - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h:1969`
**Base class:** `mod_voice_checker_base<NV, IndexClass, RuntimeTarget::GlobalModulator>`
**Classification:** control_source

## Signal Path

global_mod_gate connects to the GlobalModulatorContainer via the runtime_target system and monitors whether a specific global modulator's envelope is still playing for the current voice. It outputs a binary 0/1 modulation signal: 1.0 while the modulator is active, 0.0 when the voice has been released (ClearState::Reset). The node does not modify audio -- it only reads from the modulation signal source and outputs via ModValue/handleModulation().

## Gap Answers

### gate-signal-derivation: How active state is determined

The `mod_voice_checker_base::Data::isPlaying()` method checks the `ClearState` reset flag from the EventData:
```
if (eventData.resetFlag != nullptr)
    return eventData.resetFlag[voiceIndex] != modulation::ClearState::Reset;
return true;
```
This means: if the modulation source is an envelope type (which has resetFlags per voice), the node checks whether the voice's ClearState has reached `Reset`. If resetFlag is nullptr (VoiceStart or TimeVariant modulators), `isPlaying()` always returns true.

The `check()` method is called from both `process()` and `processFrame()`: if `isPlaying()` returns false, `mv.setModValue(0.0)` is called. On note-on, `mv.setModValue(1.0)` is set in `handleHiseEvent()`.

### runtime-target-connection: Connection mechanism

global_mod_gate inherits from `mod_voice_checker_base` which inherits from `runtime_target::indexable_target<IndexClass, RuntimeTarget::GlobalModulator, SignalSource>`. The default IndexClass is `fix_hash<1>`, matching the GlobalModulatorContainer's RuntimeSource (hash=1). This is the same runtime_target infrastructure used by core.global_mod. The `IsFixRuntimeTarget` property is registered in the `mod_voice_checker_base` constructor.

When connected, `onValue(SignalSource)` stores the signal source. On note-on, `signal.getEventData(index, event, isPolyphonic)` retrieves per-voice EventData for the selected modulator slot.

### modulation-output-type: Output type

The output is strictly binary: `mv.setModValue(1.0)` on note-on, `mv.setModValue(0.0)` when `isPlaying()` returns false. The `handleModulation(double& v)` method uses `mv.getChangedValue(v)` following the standard ModValue pattern. There are no intermediate values.

### index-to-modulator-mapping: Index parameter mapping

Yes, the Index parameter (0-16) uses the same indexing system as core.global_mod. In `setIndex()`, the value is stored as `this->index` and used in `signal.getEventData(index, event, ...)` to select which child modulator within the GlobalModulatorContainer to read from.

### polyphonic-gate-behaviour: Per-voice gate tracking

Yes, global_mod_gate tracks per-voice state. `PolyData<Data, NumVoices> state` stores one `Data` struct per voice. On note-on, `handleHiseEvent()` stores the voice-specific `EventData` and `voiceIndex = e.getEventId() % NumVoices`. Each voice's `isPlaying()` checks its own `resetFlag[voiceIndex]`, so each voice independently reports active/inactive based on the corresponding voice of the global envelope modulator.

## Parameters

- **Index:** Range 0-16, step 1, default 1. Selects which modulator slot in the GlobalModulatorContainer to monitor.

## Polyphonic Behaviour

`PolyData<Data, NumVoices> state` stores per-voice EventData and voiceIndex. Each voice independently tracks its own envelope lifecycle from the global modulator. Note: `polyphonic_base` is constructed with `addProcessEventFlag=false` -- IsProcessingHiseEvent is NOT registered, but `handleHiseEvent()` is defined and receives events through the polyphonic wrapper.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

global_mod_gate is a thin wrapper around `mod_voice_checker_base` specialized with `RuntimeTarget::GlobalModulator` and default `fix_hash<1>` indexer. The `prepare()` override only calls the base class. The node is architecturally a companion to core.global_mod: while global_mod gives the continuous envelope value, global_mod_gate gives the binary voice-active state for voice lifecycle management.
