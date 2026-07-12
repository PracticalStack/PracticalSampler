# ShapeFX - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/ShapeFX.h`, `hi_core/hi_modules/effects/fx/ShapeFX.cpp`
**Base class:** `MasterEffectProcessor`

## Signal Path

The signal path in `applyEffect()` (ShapeFX.cpp:574-665) is a single stereo chain:

1. Copy input to dry buffer, apply inverse mix gain to dry copy
2. If filters not bypassed: high-pass then low-pass on wet signal
3. Apply input gain (smoothed, 40ms)
4. Add DC bias (BiasLeft to left channel, BiasRight to right channel)
5. If LimitInput enabled: per-sample soft limiting
6. If oversampling > 1x: upsample -> shape -> bitcrush -> downsample (dry buffer delayed for latency compensation)
7. If oversampling = 1x: shape -> bitcrush
8. If filters not bypassed: DC removal (30 Hz high-pass)
9. If autogain enabled: apply autogain compensation (smoothed, 40ms)
10. Apply mix gain to wet signal
11. Sum: output = wet * Mix + dry * (1 - Mix)

## Gap Answers

### signal-path-order: What is the exact processing order in applyEffect()?

**Answer:** The order is exactly as listed in the Signal Path section above. Key observations:

- The dry buffer is copied BEFORE any processing (line 582-583), preserving the unfiltered, unbiased original signal
- Filters (HP + LP) are applied first to the wet path only (lines 590-594)
- Gain is applied after filtering but before bias and shaping (line 597)
- Bias is added after gain, before the limiter and shaper (lines 602-603)
- The soft limiter sits between bias and shaping (lines 605-617)
- Shaping and bitcrushing happen together, inside oversampling if active (lines 619-646)
- DC removal follows shaping but only when filters are active (lines 648-652)
- Autogain is applied after DC removal (lines 654-655)
- Mix is the final step: wet is scaled by Mix, then dry (pre-scaled by 1-Mix) is added (lines 660-664)

### drive-parameter-usage: How is Drive used in applyEffect()?

**Answer:** The Drive parameter is **vestigial in ShapeFX** (the MasterEffect version). In `setInternalAttribute()` (line 212), `case Drive: drive = newValue; break;` stores the value but triggers no update. The `drive` member variable is never read in `applyEffect()`. The only input gain applied is via `gainer.processBlock()` which uses the `gain` member (set from the Gain parameter in dB).

Drive IS actively used in the polyphonic version (PolyshapeFX) where it multiplies the audio signal per-sample alongside the drive modulation chain. In ShapeFX, Drive is stored and serialised but has no DSP effect.

### mode-values: What shaping functions are available for each Mode value?

**Answer:** The ShapeMode enum (ShapeFX.h:96-112) defines these modes:

| Value | Name | Description |
|-------|------|-------------|
| 1 | Linear | Passthrough (no shaping) |
| 2 | Atan | Arctangent soft clipping |
| 3 | Tanh | Hyperbolic tangent saturation |
| 4 | Sin | Sine wavefold |
| 5 | Asinh | Inverse hyperbolic sine (gentle saturation) |
| 6 | Saturate | Custom saturation curve (gain-dependent) |
| 7 | Square | Hard squaring |
| 8 | SquareRoot | Square root compression |
| 9 | TanCos | Tangent-cosine combination |
| 10 | Chebichev1 | Chebyshev polynomial (1st order harmonic) |
| 11 | Chebichev2 | Chebyshev polynomial (2nd order harmonic) |
| 12 | Chebichev3 | Chebyshev polynomial (3rd order harmonic) |
| 32 | Curve | User-defined table lookup (TableProcessor) |
| 33 | AsymetricalCurve | User-defined asymmetrical table lookup |

Note: Values 0, 13-31 are unused gaps in the enum. The Mode parameter range in metadata (0-33) includes these gaps.

### table-lookup-position: Where does the TableProcessor lookup occur?

**Answer:** The table lookup IS the shaping function for modes 32 (Curve) and 33 (AsymetricalCurve). When `mode == Curve` or `mode == AsymetricalCurve`, `shapers[mode]->processBlock()` (line 630 or 644) calls the TableShaper which reads from the lookup table. The table is edited via the TableProcessor interface in the UI. The lookup happens at the same point in the signal chain as all other shaping modes - after gain, bias, and limiting, but before DC removal and autogain.

### oversampling-latency: Does oversampling introduce latency?

**Answer:** Yes. The oversampler uses half-band polyphase IIR filters which introduce latency proportional to the oversampling factor. In `updateOversampling()` (ShapeFX.cpp:501-528):

- The latency in samples is obtained from `oversampler->getLatencyInSamples()`
- Delay lines (`lDelay`, `rDelay`) are configured to delay the dry buffer by this amount
- When oversampling > 1x and latency > 0, the dry buffer is delayed to align with the oversampled wet signal (lines 634-638)

CPU cost scales with the oversampling factor: 2x doubles the shaping work, 4x quadruples it, etc. The oversampler itself adds overhead for the up/down filtering. At 1x, no oversampler is used and no latency is introduced.

### autogain-calculation: How is autogain calculated?

**Answer:** Autogain is calculated statically in `updateGain()` (ShapeFX.cpp:530-556) whenever the Mode or Gain parameter changes:

1. Feed 128 linearly spaced input values (0 to 1, scaled by current gain) through the current shaper
2. Sum the absolute output values
3. Divide by 64 (average of 128/2 since only positive half)
4. Autogain value = 1 / sum (the inverse of the average output magnitude)

This is a configuration-time calculation, not a dynamic measurement. The autogain value is applied via a smoothed multiplier (`autogainer.processBlock`) with 40ms smoothing time to avoid clicks when switching modes.

### dc-removal: Is there DC removal after the shaper?

**Answer:** Yes, but only when filters are active. A 30 Hz high-pass filter (`lDcRemover`, `rDcRemover`) is applied after shaping and bitcrushing (lines 648-652), but it is gated by `!bypassFilters`. The DC removal filter is initialised in `prepareToPlay()` with `IIRCoefficients::makeHighPass(sampleRate, 30.0f)` (line 361-364).

If BypassFilters is enabled, the DC remover is also bypassed. This means bias-induced DC offset will persist in the output when filters are bypassed.

### mix-implementation: How is dry/wet mix implemented?

**Answer:** The dry copy is taken at the very start of `applyEffect()`, BEFORE any processing including filtering (lines 582-583). The mix uses smoothed gain ramps:

- `mixSmoother_invL/R` applies (1 - Mix) gain to the dry buffer (line 585-586)
- `mixSmootherL/R` applies Mix gain to the wet buffer (lines 660-661)
- The dry and wet are summed: `wet += dry` (lines 663-664)

So at Mix = 0: output is fully dry (original input). At Mix = 1: output is fully wet (shaped signal). The dry signal always preserves the unfiltered, unbiased original.

## Processing Chain Detail

1. **Dry buffer copy** - copy input to dry buffer, apply (1-Mix) gain. CPU: negligible.
2. **Pre-shaper filters** - HP then LP on wet signal. Per-sample IIR. Gated by !BypassFilters. CPU: low.
3. **Input gain** - smoothed gain multiply (40ms). CPU: low.
4. **DC bias** - add constant to L and R. CPU: negligible.
5. **Soft limiter** - per-sample limiting at -0.5 dB threshold. Gated by LimitInput. CPU: low.
6. **Oversampling up** - if Oversampling > 1x. CPU: medium to high (scales with factor).
7. **Waveshaping** - per-sample nonlinear function. CPU: low (math modes) to medium (table lookup). Mode-dependent.
8. **Bitcrushing** - per-sample quantisation. Gated by Reduce > 0. CPU: low.
9. **Oversampling down** - if Oversampling > 1x. CPU: medium to high.
10. **DC removal** - 30 Hz HP filter. Gated by !BypassFilters. CPU: negligible.
11. **Autogain** - smoothed gain multiply. Gated by Autogain toggle. CPU: negligible.
12. **Mix** - smoothed wet gain, then sum with pre-scaled dry. CPU: negligible.

## Conditional Behaviour

- **BypassFilters**: When On, skips pre-shaper HP/LP filters AND post-shaper DC removal. All three filters are bypassed together.
- **LimitInput**: When On, applies per-sample soft limiting before the shaper. Threshold is fixed at -0.5 dB, attack 0.03ms, release 100ms.
- **Autogain**: When On, applies static gain compensation after shaping. The compensation value is recalculated when Mode or Gain changes.
- **Oversampling**: When > 1x, wraps the shaping and bitcrushing in up/downsampling. Also delays the dry buffer to compensate for oversampler latency.
- **Reduce**: When > 0, applies bitcrushing after shaping. When 0, bitcrushing is skipped.
- **Mode**: Selects the shaping algorithm. Modes 32-33 use table lookup (TableProcessor).

## Interface Usage

**TableProcessor**: Provides one lookup table used by modes 32 (Curve) and 33 (AsymetricalCurve). The table maps input amplitude to output amplitude. When these modes are active, the table IS the shaping function - it replaces the mathematical functions used by modes 1-12. The table is edited in the ShapeFX editor UI.

## Vestigial / Notable

The **Drive** parameter is vestigial in ShapeFX. It is defined, stored, serialised, and exposed in metadata, but it is never read during audio processing. Only the Gain parameter affects the input level. Drive is functional in the polyphonic variant (PolyshapeFX) but not here.

## CPU Assessment

- **Baseline (1x oversampling, default settings):** low - a few per-sample IIR filters plus one per-sample nonlinear function
- **Oversampling scaling:** Each doubling of oversampling roughly doubles the shaping and bitcrushing cost. At 16x, the effect becomes high CPU.
- **Mode impact:** Mathematical modes (atan, tanh, sin) are similar cost. Table lookup modes are slightly cheaper per-sample but the difference is negligible.
- **Per-sample processing:** The shaper, limiter, and bitcrusher are all per-sample. Filters are per-sample IIR.

## UI Components

The editor class is `ShapeFXEditor` (created in ShapeFX.cpp:303-316). No FloatingTile content types discovered - this is a standard backend editor panel.

## Notes

- The Saturate mode (value 6) has special handling in `updateGain()` where it calls `static_cast<InternalSaturator*>(shapers[Saturate])->updateAmount(gain)` to configure the saturation amount based on the current gain.
- The oversampler uses `filterHalfBandPolyphaseIIR` type (no linear phase option exposed).
- The waveform display (WaveformComponent::Broadcaster) shows the transfer function curve, not the audio waveform. It recalculates when mode, gain, or table data changes.
