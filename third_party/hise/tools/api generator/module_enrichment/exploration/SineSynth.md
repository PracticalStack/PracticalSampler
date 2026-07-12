# SineSynth - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/synthesisers/synths/SineSynth.h` (252 lines)
- `hi_core/hi_modules/synthesisers/synths/SineSynth.cpp` (186 lines)
- `hi_core/hi_core/UtilityClasses.h` (Saturator class, lines 410-436)
- `hi_dsp_library/dsp_basics/Oscillators.h` (SineLookupTable, lines 41-76)

## Gap Answers

### signal-path-order

**Question:** What is the exact processing order in calculateBlock()?

**Answer:** The per-voice processing order in `SineSynthVoice::calculateBlock()` (SineSynth.cpp:120-184) is:

1. **Sine lookup with pitch modulation** - For each sample, read the sine table at the current phase (`voiceUptime`). If pitch modulation values exist, multiply `uptimeDelta` by the pitch modulation value per-sample. Otherwise use a fixed `uptimeDelta`.
2. **Saturation waveshaping** - If `saturationAmount > 0`, apply the saturator formula to every sample in the buffer. Note: if saturationAmount == 1.0, it is clamped to 0.99 to prevent silence.
3. **Gain modulation** - Multiply the buffer by per-voice gain modulation values (if a gain modulation chain is present) or by a constant gain value.
4. **Mono-to-stereo copy** - Copy the left channel to the right channel.
5. **Effect chain** - Render the voice through the effect chain (`effectChain->renderVoice()`).

### saturation-formula

**Question:** What waveshaping formula does the Saturator use?

**Answer:** The Saturator (UtilityClasses.h:419-422) uses a soft-clipping transfer function:

```
k = 2 * saturationAmount / (1 - saturationAmount)
output = (1 + k) * input / (1 + k * |input|)
```

This is applied per-sample after the sine lookup. At `saturationAmount = 0`, `k = 0` and the formula reduces to `output = input` (no effect). As saturation increases towards 1.0, the curve approaches hard clipping. The value is clamped to 0.999 maximum to prevent division by zero.

The formula in `calculateBlock()` recalculates `k` locally as `2 * saturation / (1 - saturation)` (line 153), which matches the Saturator class formula.

### freq-ratio-formula

**Question:** How is the frequency ratio calculated from CoarseFreqRatio and FineFreqRatio?

**Answer:** From `setInternalAttribute()` (SineSynth.h:196-209), when `useRatio` is true:

```
cToUse = CoarseFreqRatio - 1.0

if cToUse == 0:       factor = 1.0 + FineFreqRatio
if cToUse > 0:        factor = 1.0 + cToUse + FineFreqRatio
if cToUse < 0:        factor = 2^cToUse + FineFreqRatio
```

So CoarseFreqRatio=1 means the fundamental (factor near 1.0). CoarseFreqRatio=2 means factor=2.0 (one octave up). CoarseFreqRatio=3 means factor=3.0 (third harmonic). Negative values use exponential (octave-based) division. FineFreqRatio is always added linearly as a fractional offset.

When `useRatio` is false (musical mode):
```
factor = 2^(OctaveTranspose + SemiTones/12)
```

### tuning-mode-switch

**Question:** Does UseFreqRatio completely disable OctaveTranspose/SemiTones and vice versa?

**Answer:** Yes, it is a strict either/or. In `setInternalAttribute()` (SineSynth.h:194-215), the tuning factor calculation branches on `useRatio`:
- If `useRatio` is true: only CoarseFreqRatio and FineFreqRatio are used
- If `useRatio` is false: only OctaveTranspose and SemiTones are used

The resulting factor is applied to all voices via `setOctaveTransposeFactor()`. Only one tuning mode contributes to the pitch at any time.

### stereo-output

**Question:** How is the mono sine signal converted to stereo?

**Answer:** At line 181 of SineSynth.cpp:
```cpp
FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);
```

The left channel is simply copied to the right channel after gain modulation. Balance is handled by the base class (ModulatorSynth) after `calculateBlock()` returns, not within the voice itself.

### sine-lookup-table

**Question:** What size is the sine lookup table? Is interpolation linear or higher-order?

**Answer:** The table is 2048 samples (template parameter in SineSynth.h:91: `SharedResourcePointer<SineLookupTable<2048>>`). Interpolation is linear between adjacent samples (Oscillators.h:55-66). The table is shared across all SineSynth instances via `SharedResourcePointer`, so multiple SineSynth modules share a single lookup table in memory.

## Additional Findings

- The `startNote()` method (SineSynth.h:66-80) applies the global pitch factor (`getMainController()->getGlobalPitchFactor()`) to `uptimeDelta`, meaning the master tune setting affects SineSynth.
- The `voiceUptime` is initialised from `getCurrentHiseEvent().getStartOffset()`, supporting sample-accurate note start timing.
- The C++ doc comment explicitly warns against using multiple SineSynth instances for additive synthesis due to per-instance pitch modulation overhead.

## Issues

No description inaccuracies found. All base data descriptions match the C++ implementation.
