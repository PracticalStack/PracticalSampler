# core.matrix_mod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:748`
**Base class:** Custom -- inherits `display_buffer_base<true>`, `polyphonic_base`, `indexable_target<fix_hash<1>, RuntimeTarget::GlobalModulator, SignalSource>` directly. Does NOT inherit from mod_base.
**Classification:** audio_processor (modulation bridge)

## Signal Path

A dual-source modulation node that connects to the GlobalModulatorContainer with modulation matrix features. Internally contains two `global_mod` instances: `sourceMod` (Raw mode, provides the main modulation signal) and `auxMod` (Aux mode, provides dynamic intensity modulation). During processing, both internal mods are processed into separate buffers, then `applyModulation()` combines them: the aux signal scales the source signal's intensity, and the result is applied to the input audio using the selected mode (Scale/Unipolar/Bipolar).

Signal flow: GlobalModulatorContainer -> runtime_target connection -> two global_mod instances read their respective slots -> source provides raw signal, aux provides intensity signal -> matrix_mod's own applyModulation combines them with inversion and zero position -> ModValue output + audio channel 0 write.

## Gap Answers

### dual-source-interaction: How do source and aux signals combine?

In `applyModulation()` (line 844), the aux buffer (`intensity` parameter in the function) acts as a per-sample intensity multiplier for the source signal. The combination depends on mode:

- **Scale (Gain):** `mv = (1 - aux) + aux * source`, then `dst = (dst - zeroDelta) * mv + zeroDelta`. The aux signal scales how much the source affects the output.
- **Unipolar:** `src *= aux`, then `dst += src`. The aux-scaled source is added to the input.
- **Bipolar:** `src = (2*src - 1) * aux`, then `dst += src`. The bipolar source is scaled by aux intensity.

The key insight: the Intensity parameter of matrix_mod is NOT passed to the source -- it is passed to the aux mod's `setValue()` method (line 921). This means Intensity sets the base value for the aux channel's Aux-mode formula: `output = (1 - auxIntensity + auxIntensity * auxSignal) * Intensity`.

### mode-variants: Are formulas identical to global_mod's?

The formulas differ. matrix_mod's `applyModulation()` is its own implementation (line 844), NOT the mod_base version:

- **Scale (0):** Uses `(1 - aux) + aux * source` for per-sample intensity, then multiplies `dst` with zero-position offset. Different from global_mod's Gain which uses base/intensity smoothed floats.
- **Unipolar (1):** `src *= aux; dst += src` -- additive, no clamping.
- **Bipolar (2):** `src = (2*src - 1) * aux; dst += src` -- bipolar additive, no clamping.

Notably, matrix_mod does NOT clamp output to [0,1] for any mode. Also, "Scale" is the label (not "Gain") to reflect the different formula.

### inversion-behaviour: When does inversion apply?

Inversion applies BEFORE the mode formula, at the top of `applyModulation()` (line 847): `src[i] = 1.0f - src[i]`. This inverts only the source signal, not the aux signal. The inverted source then flows into whichever mode formula is active.

### zero-position-values: What are Left and Center?

In `setZeroPosition()` (line 940): Left (0) sets `zeroDelta = 0.0f`, Center (1) sets `zeroDelta = 0.5f`. In the Scale mode formula (line 862-865): `dst -= zeroDelta; dst *= mv; dst += zeroDelta`. When zeroDelta=0.5 (Center), the modulation is centered around 0.5 instead of 0.0. This is equivalent to the `useMidPositionAsZero` concept but implemented directly. Only affects Scale mode -- Unipolar and Bipolar do not use zeroDelta.

### source-index-range: Why is the range -1 to 64?

SourceIndex and AuxIndex use range `{-1.0, NumMaxModulationSources, 1.0}` where NumMaxModulationSources is HISE_NUM_MODULATORS_PER_CHAIN (typically 128, but the JSON shows max 64). Default is -1 (disconnected). The -1 default means the node starts with no source connected -- the user must explicitly select a modulator. This is different from global_mod (default 0, always connected to first slot). The wider range accommodates the GlobalModulatorContainer potentially having many children.

### aux-intensity-mode: How does AuxIntensity control the aux channel?

`setAuxIntensity()` calls `auxMod.setIntensity()`, which sets the per-voice intensity smoothed float on the internal aux global_mod. The aux global_mod uses `internal_config<TargetMode::Aux>`. In the Aux formula: `output = (1 - intensity + intensity * signal) * baseValue`. So AuxIntensity controls how much the aux modulator signal affects the intensity scaling. At AuxIntensity=0, the aux output equals baseValue. At AuxIntensity=1, the aux output is fully modulated by the aux signal.

Meanwhile, the matrix_mod Intensity parameter calls `auxMod.setValue()` (line 921), setting the aux mod's baseValue. This means Intensity acts as the overall base intensity that the aux signal modulates.

### envelope-voice-tracking: How does matrix_mod handle voice lifecycle?

The `isPlaying()` method (line 988) checks the source mod's EventData: if the source is an Envelope type, it checks `thisBlockSize != nullptr && *thisBlockSize > 0`. When the envelope has released and its block size drops to 0, `isPlaying()` returns false. For non-envelope sources, it always returns true. This method is available for external callers (e.g., voice management) to determine if the voice should remain active.

## Parameters

- **SourceIndex** (-1 to NumMaxModulationSources, step 1, default -1): Selects the primary modulation source from the GlobalModulatorContainer. -1 = disconnected.
- **Intensity** (-1 to 1, default 0.0): Base intensity value passed to the aux channel's Value parameter. Controls overall modulation depth.
- **Mode** (Scale/Unipolar/Bipolar, default Scale): Selects how source+aux combine with the input signal.
- **Inverted** (Normal/Inverted, default Normal): Flips the source signal (1 - signal) before mode application.
- **AuxIndex** (-1 to NumMaxModulationSources, step 1, default -1): Selects the auxiliary modulation source. -1 = disconnected.
- **AuxIntensity** (0-1, default 0.0): Controls how much the aux modulator signal affects the intensity scaling.
- **ZeroPosition** (Left/Center, default Left): Sets the modulation center point. Center subtracts 0.5 before and adds 0.5 after Scale mode multiplication.

## Conditional Behaviour

Mode parameter selects three formula variants in the local `applyModulation()`:
- Scale: Multiplicative with aux-based intensity, with zero-position offset
- Unipolar: Additive with aux-scaled source
- Bipolar: Bipolar additive with aux-scaled source

ZeroPosition only affects Scale mode (zeroDelta is applied within the Scale branch only).

Inverted flag applies universally to the source signal before any mode formula.

## Polyphonic Behaviour

Per-voice state is managed by the two internal `global_mod` instances, each with their own `PolyData<Data, NV>`. On note-on, `handleHiseEvent()` stores the current voice index and forwards the event to both sourceMod and auxMod. The `lastVoiceIndex` is used to gate display buffer updates to the triggering voice only.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: [{"parameter": "Mode", "impact": "negligible", "note": "All modes have similar per-sample cost"}]

## Notes

- matrix_mod uses `fix_hash<1>` (matching GlobalModulatorContainer's RuntimeSource hash = 1), so it always connects to the GlobalModulatorContainer. The internal global_mod instances also use `fix_hash<1>`.
- The internal global_mod instances use `internal_config` with `shouldProcessSignal() = true` (always writes to their mono buffer) and `shouldEnableDisplayBuffer() = false`.
- Both internal mods use `internal_config` which has `SN_EMPTY_PREPARE` and `SN_EMPTY_INITIALISE`, meaning their config objects skip prepare/initialise -- the outer matrix_mod handles these directly.
- Unlike global_mod, matrix_mod does NOT clamp output to [0,1]. Downstream nodes must handle potential out-of-range values.
- The `sourceBuffer` and `auxBuffer` are `heap<float>` allocated to blockSize. The `sptr`/`aptr` raw pointers are used for ProcessData construction via `makeProcessData()`.
- Default Intensity = 0.0 means the node starts with no modulation effect -- the user must increase Intensity to hear the modulation.
- The SourceIndex/AuxIndex max range in the JSON shows 64, but the C++ uses `NumMaxModulationSources` which defaults to HISE_NUM_MODULATORS_PER_CHAIN (could be 128). The JSON may have a stale value.
