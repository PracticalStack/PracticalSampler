# Constant - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/ConstantModulator.h`, `hi_core/hi_modules/modulators/mods/ConstantModulator.cpp`
**Base class:** `VoiceStartModulator` (which extends `Modulator` + `VoiceModulation`)

## Signal Path

noteOn -> `handleHiseEvent()` calls `calculateVoiceStartValue()` -> stores `unsavedValue` -> `startVoice()` returns `unsavedValue` -> intensity applied by modulation chain framework -> output

The module itself is stateless. The entire signal path is:

1. On note-on, `calculateVoiceStartValue()` returns a fixed value depending on mode:
   - **GainMode:** returns `0.0f` (the intensity slider carries the actual value)
   - **PitchMode / PanMode / GlobalMode:** returns `1.0f`
2. The base class `VoiceStartModulator::handleHiseEvent()` stores this in `unsavedValue`.
3. On `startVoice()`, `unsavedValue` is copied into the per-voice array and returned.
4. The modulation chain framework applies intensity via `calcIntensityValue()` / `applyModulationValue()`.

## Gap Answers

### constant-output-value: Does calculateVoiceStartValue() always return exactly 1.0?

No. The return value is mode-dependent:

```
return (getMode() == GainMode) ? 0.0f : 1.0f;
```

In **GainMode**, it returns `0.0f`. The code comment explains: "Returns 0.0f and let the intensity do its job." This means in GainMode the intensity slider (default 1.0, range 0.0-1.0) IS the output value. The `calculateVoiceStartValue` result is effectively a neutral base that the intensity modifies.

In **PitchMode** (and other modes), it returns `1.0f`, which is the neutral pitch multiplier. Intensity then scales around this value.

There is no hidden state. `setInternalAttribute()` is a no-op (empty body). `getAttribute()` always returns `0.0f`. The `SpecialParameters` enum is empty (`numTotalParameters` = 0).

### constant-use-case: What is the intended use case for a constant 1.0 modulator?

The Constant modulator serves as a **scriptable intensity carrier**. Its raw output is fixed, but the inherited `setIntensity()` / `getIntensity()` methods from the `Modulation` base class allow scripts to control the effective modulation value at runtime.

Primary use cases (confirmed by forum patterns and C++ design):
1. **Script-controlled gain:** Add a Constant to a gain chain, then call `Modulator.setIntensity(value)` from script to control volume while voices are playing.
2. **Script-controlled pitch:** Same pattern for pitch modulation.
3. **Multiple independent controls:** Stack multiple Constant modulators to have independent script-controllable sources (e.g., one for a knob, one for a keyswitch).

The module has no parameters of its own; all control flows through the inherited intensity mechanism.

## Processing Chain Detail

1. **calculateVoiceStartValue** (per-voice, on note-on): Returns mode-dependent constant. CPU: negligible.
2. **Intensity application** (handled by modulation chain, not by this module): Multiplies or offsets the raw value. CPU: negligible.

That is the entire processing chain. No buffers, no calculation, no iteration.

## Modulation Points

None. The Constant modulator has no child processor chains (`getNumChildProcessors()` returns 0 via VoiceStartModulator's final override). It cannot host sub-modulators.

The module IS a modulation source. Its output is controlled exclusively through the inherited intensity parameter.

## Conditional Behavior

Single conditional in `calculateVoiceStartValue()`:
- `getMode() == GainMode` -> returns `0.0f`
- All other modes -> returns `1.0f`

The mode is set by the parent modulation chain, not by the user directly.

## Vestigial / Notable

- `setInternalAttribute()` has an empty body -- any setAttribute call is silently ignored.
- `getAttribute()` always returns `0.0f` regardless of input index.
- These are not bugs; the module genuinely has no parameters.

## CPU Assessment

**Overall: negligible.** The module performs a single ternary comparison per note-on. No per-sample processing, no buffers, no allocations.

## UI Components

Uses `EmptyProcessorEditorBody` -- no custom editor, no FloatingTile content types.

## Notes

- The forum insight claiming `getIntensity()` does not exist is incorrect for current HISE. The scripting API wrapper `ScriptingModulator::getIntensity()` is registered (see `ScriptingApiObjects.cpp:2911`). In PitchMode it returns `intensity * 12.0f`; in GainMode it returns the raw intensity value.
- The description in `createMetadata()` says "constant modulation signal (1.0)" which is only accurate for PitchMode. In GainMode the raw output is 0.0 and intensity carries the value. This is technically misleading but functionally correct since the end result (with default intensity 1.0) is unity gain.
