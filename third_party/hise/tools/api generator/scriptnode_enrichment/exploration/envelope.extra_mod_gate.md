# envelope.extra_mod_gate - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h:1990`
**Base class:** `mod_voice_checker_base<NV, IndexClass, RuntimeTarget::ExternalModulatorChain>`
**Classification:** control_source

## Signal Path

extra_mod_gate connects to an extra modulation chain via the runtime_target system (ExternalModulatorChain) and monitors whether the modulator is still playing for the current voice. Like global_mod_gate, it outputs a binary 0/1 modulation signal based on the ClearState of the per-voice envelope data. The node does not modify audio.

## Gap Answers

### gate-signal-derivation: How active state is determined

Identical mechanism to global_mod_gate. The shared `mod_voice_checker_base::Data::isPlaying()` checks `eventData.resetFlag[voiceIndex] != modulation::ClearState::Reset`. If resetFlag is nullptr (non-envelope modulators), always returns true. The `check()` method outputs `mv.setModValue(0.0)` when not playing.

### runtime-target-connection: Connection mechanism

extra_mod_gate uses `RuntimeTarget::ExternalModulatorChain` with default indexer `modulation::config::ExtraIndexer` (which is `fix_hash<5000>`). This matches the `ModulatorChain::ExtraModulatorRuntimeTargetSource` with hash 5000 (CustomOffset). This is the same hash used by core.extra_mod.

### shared-implementation-with-global-gate: Shared base class

Yes, global_mod_gate and extra_mod_gate share the exact same base class template `mod_voice_checker_base<NV, IndexClass, TargetType>`. They differ only in:
- **TargetType:** `GlobalModulator` vs `ExternalModulatorChain`
- **Default IndexClass:** `fix_hash<1>` vs `ExtraIndexer` (fix_hash<5000>)
- **HISE source:** GlobalModulatorContainer vs ExtraModulatorRuntimeTargetSource

All logic -- handleHiseEvent, process, check, isPlaying, createParameters, setIndex -- is identical in the base class. The derived classes are minimal wrappers adding only the node ID, description, and prepare override.

### modulation-output-type: Output type

Strictly binary 0/1, identical to global_mod_gate. Uses the same ModValue pattern.

### polyphonic-gate-per-voice: Per-voice tracking

Yes, identical to global_mod_gate. `PolyData<Data, NumVoices> state` stores per-voice EventData with independent ClearState tracking per voice.

## Parameters

- **Index:** Range 0-16, step 1, default 1. Selects which extra modulation chain to monitor.

## Polyphonic Behaviour

Same as global_mod_gate: `PolyData<Data, NumVoices>` with per-voice EventData storage.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

extra_mod_gate is architecturally identical to global_mod_gate except for the runtime target type and hash. It is the companion to core.extra_mod, providing binary voice lifecycle information for extra modulation chains. The `ExtraIndexer` alias resolves to `fix_hash<5000>` per the modulation config infrastructure.
