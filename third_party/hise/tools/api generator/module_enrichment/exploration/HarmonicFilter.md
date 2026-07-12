# HarmonicFilter - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/HarmonicFilter.h`, `hi_core/hi_modules/effects/fx/HarmonicFilter.cpp`
**Base class:** `VoiceEffectProcessor`, `BaseHarmonicFilter` (which extends `SliderPackProcessor`)

## Signal Path

voice audio input -> interpolate per-band gain from slider packs A/B using crossfade value -> update peak filter band gains -> process through series peak filters -> voice audio output.

On `startVoice()`: the voice's MIDI note frequency (with SemitoneTranspose applied) sets the base frequency for the filter bank. Each filter band targets a successive integer harmonic (f, 2f, 3f, ...). The per-band gain values are linearly interpolated between slider pack A and slider pack B using the crossfade value (optionally modulated by the X-Fade chain). All peak filters are applied in series per-sample to the stereo buffer.

## Gap Answers

### signal-path-order

In `applyEffect()` (line 203), the processing order is:

1. Read the X-Fade modulation chain value (or use `currentCrossfadeValue` if no modulation active). This is a single value per block via `getOneModulationValue()`.
2. If this is the last started voice, update `setCrossfadeValue()` to refresh the Mix slider pack display.
3. Loop over `numBands`: linearly interpolate between `dataA->getValue(i)` and `dataB->getValue(i)` using the crossfade value to get each band's gain. Call `filterBank.updateBand(i, gainValue)` to set the gain on each filter state.
4. Call `filterBank.processSamples(b, startSample, numSamples)` which applies all active peak filters in **series** - each filter processes the sample in-place before the next filter sees it. There is no dry/wet mix; the output is the fully filtered signal only.

The filters are applied in series (not parallel). The `processSamples()` method iterates per-sample, and for each sample iterates through all active filter states, each modifying the sample in place via `input += _m1 * _v1`.

### harmonic-frequency-calculation

In `startVoice()` (line 191), the voice frequency is derived from a copy of the HiseEvent with `semiToneTranspose` added to the event's transpose amount. Then `copy.getFrequency()` converts the transposed MIDI note to a frequency.

In `PeakFilterBand::updateBaseFrequency()` (header line 172), the frequencies are strictly integer harmonics:
- `freqToUse` starts at `baseFrequency` (the fundamental)
- For each band `i`, the filter is set to `freqToUse`, then `freqToUse += baseFrequency`
- So band 0 = f, band 1 = 2f, band 2 = 3f, etc.

Bands whose frequency exceeds `sampleRate * 0.4` (Nyquist safety limit) are automatically excluded via `numBandsToUse = jmin(numBands, bandsUntilLimit)`.

SemitoneTranspose shifts the base frequency by adding semitones to the event's transpose amount before frequency conversion. It shifts the entire harmonic series up or down.

### ab-crossfade-implementation

The crossfade interpolates the per-band gain values **before** filtering, not the audio output. In `applyEffect()` (line 216):

```
gainValue = Interpolator::interpolateLinear(dataA->getValue(i), dataB->getValue(i), xModValue)
```

Only one filter bank runs per voice. The interpolated gain values are fed to `filterBank.updateBand()` which sets the gain on each filter state. This is a **parameter morph**, not a signal blend - there is no doubled CPU cost from running two filter banks.

Two separate slider packs (dataA and dataB) are maintained. A third slider pack (dataMix) is a display-only pack that shows the interpolated result, updated in `setCrossfadeValue()` for UI feedback.

### sliderpack-usage

`BaseHarmonicFilter` extends `SliderPackProcessor` initialized with 3 slider packs (line 280: `SliderPackProcessor(mc, 3)`):
- Pack 0 (dataA): slider pack A configuration
- Pack 1 (dataB): slider pack B configuration
- Pack 2 (dataMix): computed mix result (display only)

The slider pack values represent **gain in dB**, with range -24.0 to +24.0, step 0.1 (set in constructor lines 89-91). These dB values are passed to `State::setGain(gainDb)` which converts them: `_A = pow(10.0, gainDb / 40.0)`. At 0 dB, the filter has unity gain (no boost/cut). Positive values boost the harmonic, negative values cut it.

The number of sliders in each pack matches `numBands` - resized when `setNumFilterBands()` is called.

### numfilterbands-mapping

Yes, it is a power-of-two mapping. `getNumBandForFilterBandIndex()` (line 254) uses a switch:
- OneBand (0) -> 1
- TwoBands (1) -> 2
- FourBands (2) -> 4
- EightBands (3) -> 8
- SixteenBands (4) -> 16

So index `n` maps to `2^n` bands. The parameter value stored externally is `filterBandIndex + 1` (1-5), and `setInternalAttribute` subtracts 1 before passing to `setNumFilterBands`.

Changing the band count calls `PeakFilterBand::setNumBands()` which does `reset()` (clears all filter states), so there will be a brief transient when changing band count during playback. The slider packs are also resized (`setNumSliders`).

### qfactor-behavior

Q is used directly as the filter Q value. In `PeakFilterBand::State::calculateFrequency()` (header line 130), `q` is stored directly. In `updateGainInternal()` (line 142):

```
k = 1.f / (q * _A)
```

Higher Q = narrower peak bandwidth. The range 4-48 (integer steps) is applied uniformly to all bands - `PeakFilterBand::setQ()` stores a single `q` value that is used for all filter states when `calculateFrequency()` is called on each band.

The Q is not transformed - the raw integer value is used directly in the SVF coefficient calculation.

### crossfade-modulation-resolution

The X-Fade Modulation chain is applied **per-block** (not per-sample). In `applyEffect()` line 207:

```
const float xModValue = useModulation ? modChains[XFadeChain].getOneModulationValue(startSample) : currentCrossfadeValue;
```

`getOneModulationValue()` returns a single value for the entire block. This value **multiplies** the CrossfadeValue parameter (the modulation mode is `ScaleOnly` as specified in `createMetadata()` line 68). So the modulation scales the crossfade position.

Smooth real-time morphing is possible but at block-rate granularity, not sample-rate. Abrupt parameter changes may cause small discontinuities (no smoothing/interpolation of filter coefficients between blocks).

### voice-frequency-source

In `startVoice()` (line 191), the frequency is obtained from the HiseEvent:

```
HiseEvent copy(e);
copy.setTransposeAmount(copy.getTransposeAmount() + semiToneTranspose);
auto freq = copy.getFrequency();
```

The frequency is set **once at voice start** from the MIDI note number (plus transpose). It is NOT updated during the voice's lifetime - there is no `updateBaseFrequency()` call in `applyEffect()`. This means pitch bend and pitch modulation do NOT affect the harmonic filter tuning after the note starts. The filter frequencies are fixed for the voice's duration.

### performance-per-voice

The filters are **SVF (State Variable Filter)** peak EQ implementations processed **per-sample**. Each filter state's `process()` involves ~10 multiply/add operations per sample per band.

With 16 bands active, that's 16 SVF evaluations per sample per voice. For stereo processing:
- Non-SSE path: processes mono then copies to right channel (`right = left` in the non-SSE path, line 119)
- SSE path: uses SIMD to process both channels simultaneously

The `numBandsToUse` is clamped to exclude bands above Nyquist safety (0.4 * sampleRate), which provides automatic optimization for high-frequency notes.

No oversampling, no FFT. The gain coefficient update (`setGain`/`updateGainInternal`) uses a `dirty` flag to avoid recalculation when the gain hasn't changed.

Per-voice with 16 bands is moderately expensive. Each additional voice multiplies the cost linearly.

## Processing Chain Detail

1. **Voice start frequency calculation** (once per voice start): MIDI note + transpose -> base frequency. Sets all harmonic filter center frequencies. Low importance for per-block understanding.
2. **X-Fade modulation read** (per-block): reads one modulation value from the X-Fade chain, or uses the stored crossfade parameter value. CPU: negligible.
3. **Per-band gain interpolation** (per-block, per-band): linearly interpolates A/B slider pack values using crossfade position. Updates each filter's gain coefficient (with dirty-check optimization). CPU: negligible.
4. **Series peak filter processing** (per-sample, per-band): all active peak filters applied in series to stereo buffer. Each filter is an SVF peak EQ. CPU: scales linearly with band count - low (1-2 bands), medium (4-8 bands), high (16 bands).

## Modulation Points

- **X-Fade Modulation chain** (index 0): scales the Crossfade parameter value per-block. Applied via `getOneModulationValue()` in `applyEffect()`. The result is used to interpolate between slider packs A and B. Mode is ScaleOnly (multiplicative).

## Conditional Behavior

- **NumFilterBands**: controls how many peak filters are active (1, 2, 4, 8, or 16). Directly scales CPU cost. Also resizes the slider packs and resets filter states.
- **Nyquist clamping**: `updateBaseFrequency()` automatically reduces `numBandsToUse` if higher harmonics exceed `sampleRate * 0.4`. For a note at 1000 Hz at 44100 Hz sample rate, the limit is ~17 bands; for a note at 4000 Hz, only ~4 bands will be active regardless of the NumFilterBands setting.
- **Modulation active check**: `shouldBeProcessedAtAll()` determines whether to read the modulation chain or use the static parameter value.

## Interface Usage

### SliderPackProcessor

Three slider packs are registered via `BaseHarmonicFilter(mc)` which calls `SliderPackProcessor(mc, 3)`:
- **Pack 0 (A)**: source gain values in dB (-24 to +24) for each harmonic band
- **Pack 1 (B)**: alternate gain values in dB (-24 to +24) for each harmonic band
- **Pack 2 (Mix)**: computed interpolation result (read-only display) - updated in `setCrossfadeValue()` for UI visualization

In the signal path, packs A and B are read in `applyEffect()` to compute per-band gain via linear interpolation. Pack Mix is written to (not read from) in the audio path - it only updates the UI.

## CPU Assessment

- **Baseline (1 band):** low - single SVF peak EQ per sample
- **4 bands:** medium - 4 SVFs in series per sample per voice
- **8 bands:** medium-high - 8 SVFs in series per sample per voice
- **16 bands:** high - 16 SVFs in series per sample per voice
- **Scaling factor:** NumFilterBands directly scales cost; polyphony multiplies cost linearly
- **Optimization:** dirty-flag on gain updates avoids unnecessary coefficient recalculation; Nyquist clamping reduces active bands for high notes

## UI Components

Uses `HarmonicFilterEditor` - a custom Introjucer-generated editor with three SliderPack components (A, B, Mix), a ComboBox for filter band count, sliders for Q and semitone transpose, and a bipolar crossfade slider. No FloatingTile content types found.

## Notes

- The non-SSE path in `PeakFilterBand::State::process()` copies left to right (`right = left`, line 119), making the output mono. The SSE path processes both channels independently via SIMD. This means on non-SSE builds, the filter produces mono output even for stereo input.
- The filter type is an SVF (State Variable Filter) peak EQ using the Vadim Zavalishin topology (trapezoidal integration). The coefficients `_a1, _a2, _a3, _m1` are the standard SVF bell/peak form. The `_A` value is `10^(gainDb/40)` which is the square root of voltage gain, standard for peaking EQ.
- Voice frequency is fixed at note-on. No pitch tracking occurs during the voice lifetime, so pitch bend does not affect filter tuning.
- The dataMix slider pack is purely for display - its values are never read in the audio processing path.
