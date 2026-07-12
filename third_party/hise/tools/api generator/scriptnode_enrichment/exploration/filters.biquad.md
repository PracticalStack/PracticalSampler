# filters.biquad -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:163` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:408` (StaticBiquadSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<StaticBiquadSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a JUCE `IIRFilter` (second-order biquad, Direct Form II). One `IIRFilter` instance per channel. Coefficients are computed via JUCE's `IIRCoefficients` static factory methods based on the current mode.

Processing delegates to `IIRFilter::processSamples()` (block path) or `IIRFilter::processSingleSampleRaw()` (frame path), which are standard Direct Form II implementations.

## Gap Answers

### biquad-mode-values: What do Mode values 0-5 correspond to?

From `StaticBiquadSubType::getModes()`: `{ "LowPass", "High Pass", "Low Shelf", "High Shelf", "Peak", "Reso Low" }`.

- 0 = LowPass (no resonance, Butterworth-style via `makeLowPass(sr, f)`)
- 1 = High Pass (no resonance, via `makeHighPass(sr, f)`)
- 2 = Low Shelf (uses Q and Gain, via `makeLowShelf(sr, f, q, gain)`)
- 3 = High Shelf (uses Q and Gain, via `makeHighShelf(sr, f, q, gain)`)
- 4 = Peak (parametric bell, uses Q and Gain, via `makePeakFilter(sr, f, q, gain)`)
- 5 = Reso Low (resonant low pass, uses Q, via `makeLowPass(sr, f, q)`)

### biquad-vs-svf: Implementation difference?

`filters.biquad` uses JUCE's `IIRFilter` (Direct Form II biquad). Coefficients are computed once per block/64-frames via JUCE's `IIRCoefficients` factory methods (Robert Bristow-Johnson Audio EQ Cookbook formulas). The filter has 6 modes including shelving and peaking types that respond to Gain.

`filters.svf` uses a custom state variable filter with trapezoidal integration. It has different modes (LP, HP, BP, Notch, Allpass) and ignores Gain entirely.

Key trade-offs:
- Biquad has more mode variety (6 vs 5) and supports gain-dependent modes
- SVF is more stable under rapid frequency modulation (trapezoidal integration vs coefficient discontinuities)
- Biquad mode 0 (LowPass) has no resonance; use mode 5 (Reso Low) for resonant LP

### biquad-stability: Can biquad become unstable with fast modulation?

The biquad uses Direct Form II which is susceptible to coefficient discontinuities during rapid parameter changes. The Smoothing parameter (via `LinearSmoothedValue` in MultiChannelFilter) mitigates this by interpolating frequency/Q/gain values. However, with Smoothing=0 and very fast frequency modulation, coefficient jumps can cause transient instability (clicks, pops). SVF is inherently more stable for modulation.

Note: `setType()` calls `reset(numChannels)` which clears the filter state, potentially causing a click when changing modes at runtime.

### description-accuracy: Confirm characterisation.

Accurate description: "Second-order biquad (IIR) filter with LowPass, HighPass, LowShelf, HighShelf, Peak, and ResoLow modes".

## Parameters

- **Frequency:** 20-20000 Hz. Cutoff/center frequency. Smoothed.
- **Q:** 0.3-9.9. Resonance/bandwidth. Used by LowShelf (Q=slope), HighShelf, Peak (Q=bandwidth), ResoLow (Q=resonance). Ignored by LowPass and HighPass modes. Smoothed.
- **Gain:** -18 to +18 dB. Boost/cut. Used by LowShelf, HighShelf, Peak modes. Ignored by LowPass, HighPass, ResoLow. Smoothed.
- **Smoothing:** 0-1 seconds. Coefficient interpolation time.
- **Mode:** 0-5 integer. Selects filter type.
- **Enabled:** 0 or 1. Hard bypass.

## Conditional Behaviour

Each mode selects a different `IIRCoefficients` factory method. Modes 0-1 ignore Q and Gain. Modes 2-4 use Q and Gain. Mode 5 uses Q but ignores Gain.

Changing Mode triggers `reset(numChannels)` which clears all filter state (potential click on mode switch during playback).

## Polyphonic Behaviour

Same as all FilterNodeBase nodes: `PolyData<FilterObject, NumVoices>` stores one `MultiChannelFilter<StaticBiquadSubType>` per voice.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

JUCE IIRFilter is highly optimized. Block processing uses `processSamples()` which is a tight inner loop.
