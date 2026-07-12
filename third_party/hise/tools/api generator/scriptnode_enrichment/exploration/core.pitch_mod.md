# core.pitch_mod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:690`
**Base class:** `mod_base<NV, PitchIndexer, RuntimeTarget::ExternalModulatorChain, pitch_config>` (directly, NOT through indexable_mod_base)
**Classification:** audio_processor (modulation bridge)

## Signal Path

Picks up the pitch modulation signal from the parent sound generator's pitch modulation chain and exposes it inside scriptnode. Uses `fix_hash<90001>` (PitchModulation constant) to automatically connect to the pitch chain -- no Index parameter needed. The signal is passed through in Pitch mode (no transformation), outputting raw pitch factor values. The `useMidPositionAsZero` flag is true but only affects Gain mode, which pitch_mod never uses -- so it has no effect on the actual output.

Signal flow: Parent sound generator's pitch modulation chain -> ModulatorChain::RuntimeTargetSource -> runtime_target connection -> SignalSource -> EventData query per voice -> pitch factor passthrough -> ModValue output (unnormalised).

## Gap Answers

### pitch-factor-range: What is the typical output range?

The output is raw pitch factor values from the HISE pitch modulation chain. Typical range: 1.0 = no pitch change, 0.5 = octave down, 2.0 = octave up. These are the same pitch ratio values used throughout HISE's modulation system (computed from semitone/cent offsets). The values are NOT clamped to [0,1] -- Pitch mode in `applyModulation()` returns immediately without any processing (line 189).

### mid-position-zero: How does useMidPositionAsZero affect pitch_mod?

The `pitch_config` sets `useMidPositionAsZero() = true`, but this flag is only checked inside the Gain mode branch of `applyModulation()` (lines 159-165). Since pitch_mod always uses TargetMode::Pitch which returns immediately, the flag has no effect on the actual modulation math. It exists for display/configuration consistency but is functionally irrelevant for pitch_mod's output.

### display-transform: What does pitchFactorToNormalisedRange do?

In `setExternalData()` (line 727), pitch_mod installs `ModBufferExpansion::pitchFactorToNormalisedRange` as a `transformFunction` on the ModPlotter's property object. This converts raw pitch factor values to a [0,1] range for display purposes only. The actual modulation output remains unnormalised pitch factors. This ensures the ModPlotter shows a visually sensible waveform rather than values centered around 1.0.

### fixed-connection: Does pitch_mod automatically connect?

Yes. The `fix_hash<90001>` indexer has `mustBeConnected() = true` and a compile-time hash matching the PitchModulation constant (90001). When the runtime_target system iterates connections, any ModulatorChain::RuntimeTargetSource with the PitchModulation hash will automatically match. If the parent sound generator has no pitch modulation chain, no connection is established and the SignalSource remains uninitialised (`ok = false`), causing all process methods to skip processing. The output defaults to 0.0 (no pitch change is not correctly represented -- see Notes).

## Parameters

- **ProcessSignal** (Disabled/Enabled, default Disabled): When enabled, writes the pitch modulation signal to audio channel 0. This is the only parameter.

## Polyphonic Behaviour

Same per-voice state as global_mod (`PolyData<Data, NV>`). Each voice receives its own EventData from the pitch modulation chain, supporting per-voice pitch modulation (e.g., per-voice LFO pitch vibrato).

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: [{"parameter": "ProcessSignal", "impact": "low", "note": "Enabled mode writes full buffer; disabled evaluates single sample per block"}]

## Notes

- pitch_mod does NOT go through `indexable_mod_base` and therefore does NOT register the `NeedsModConfig` property. This is the key structural difference from global_mod and extra_mod.
- The `isNormalisedModulation()` returns false (via pitch_config), so downstream parameter connections receive raw pitch factor values without range conversion.
- The `static_assert` enforces that the Indexer must be PitchIndexer (`fix_hash<90001>`).
- When no pitch modulation chain exists, the output defaults to the base value from the smoothed float (which defaults to 0.0), not to 1.0 (no pitch change). This could be a subtle issue if downstream code expects 1.0 as the identity pitch factor.
