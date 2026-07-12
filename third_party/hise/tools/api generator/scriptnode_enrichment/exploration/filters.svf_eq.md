# filters.svf_eq -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:169` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:551` (StateVariableEqSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<StateVariableEqSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a second-order SVF designed for parametric EQ use. Unlike `filters.svf`, this implementation uses Vadim Zavalishin's TPT (topology-preserving transform) SVF for ALL modes, not just allpass. It maintains two state variables per channel (`_ic1eq`, `_ic2eq`) and computes output as a linear combination `m0*v0 + m1*v1 + m2*v2` where the m[] coefficients determine the filter type.

The coefficient computation includes per-sample smoothing: the `tick()` method interpolates `a[]` and `m[]` coefficients toward their targets with a fixed 0.99 gain factor (approximately 1ms convergence). This provides built-in zipper-free coefficient changes independent of the Smoothing parameter.

Processing iterates per-sample: outer loop over samples, inner loop over channels. Each sample calls `coefficients.tick()` then `states[c].tick()` for each channel.

## Gap Answers

### svf-eq-mode-values: What do Mode values 0-4 correspond to?

From `StateVariableEqSubType::getModes()`: `{ "LowPass", "HighPass", "LowShelf", "HighShelf", "Peak" }`.

- 0 = LowPass (resonant low pass)
- 1 = HighPass
- 2 = LowShelf (gain-dependent)
- 3 = HighShelf (gain-dependent)
- 4 = Peak (parametric EQ bell, gain-dependent)

### svf-eq-vs-svf-difference: What is the architectural difference?

`filters.svf` uses a trapezoidal-integration SVF (Simper) for LP/HP/BP/Notch modes, with a separate Zavalishin formulation only for Allpass. Its modes are classic filter types (LP, HP, BP, Notch, Allpass) and it ignores the Gain parameter entirely.

`filters.svf_eq` uses Zavalishin's TPT SVF uniformly for ALL modes, with a mixing matrix `m[3]` that determines the filter shape. Its modes are EQ-oriented (LowPass, HighPass, LowShelf, HighShelf, Peak), and the Gain parameter is used by LowShelf, HighShelf, and Peak modes to control boost/cut. It also has built-in per-sample coefficient smoothing via the `tick()` method (independent of the Smoothing parameter).

Key implementation differences:
- svf uses `float` state variables; svf_eq uses `double` state variables
- svf has separate code paths per mode in processSamples; svf_eq has one unified path using the m[] mixing coefficients
- svf_eq's coefficient smoothing (0.99 factor per sample) provides additional smoothing on top of MultiChannelFilter's LinearSmoothedValue

### svf-eq-gain-relevance: Which modes use the Gain parameter?

The gain flows through `updateCoefficients` which calls `setGain(Decibels::gainToDecibels(gain))` then `update()`. In `update()`:
- **LowPass:** m = {0, 0, 1} -- Gain has no effect
- **HighPass:** m = {1, -k, -1} -- Gain has no effect (k = 1/q)
- **LowShelf:** g /= gain_sqrt; m = {1, k*(gain-1), gain*gain-1} -- Gain USED
- **HighShelf:** g *= gain_sqrt; m = {gain*gain, k*(1-gain)*gain, 1-gain*gain} -- Gain USED
- **Peak:** m = {1, k*(gain*gain-1), 0} -- Gain USED

Note: gain is computed as `pow(10, gainDb/40)` (fourth root of linear gain), and gain_sqrt = sqrt(gain). This is the standard parametric EQ gain convention.

### description-accuracy: Confirm characterisation.

Accurate description: "SVF-based parametric EQ with LowPass, HighPass, LowShelf, HighShelf, and Peak modes". The phase3 doc typo "filder" should be "filter".

## Parameters

- **Frequency:** 20-20000 Hz. Cutoff/center frequency. Smoothed.
- **Q:** 0.3-9.9. Bandwidth control. For Peak mode, higher Q = narrower bell. k = 1/q (or 1/(q*gain) for Peak). Smoothed.
- **Gain:** -18 to +18 dB. Boost/cut amount for LowShelf, HighShelf, Peak modes. Ignored by LowPass and HighPass. Smoothed.
- **Smoothing:** 0-1 seconds. Controls MultiChannelFilter's LinearSmoothedValue ramp time. Additional per-sample smoothing (0.99 factor) always active.
- **Mode:** 0-4 integer. Selects EQ band type.
- **Enabled:** 0 or 1. Hard bypass.

## Polyphonic Behaviour

Same as filters.svf: `PolyData<FilterObject, NumVoices>` stores one filter per voice.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Per-sample processing with double-precision arithmetic. The per-sample `tick()` coefficient interpolation adds slight overhead vs filters.svf, but the unified processing path (no per-mode switch in the inner loop) partially compensates.

## Notes

svf_eq is the preferred choice for parametric EQ applications where Gain control matters (shelving, peaking). For simple LP/HP/BP/Notch filtering where Gain is not needed, filters.svf is sufficient. The double-precision state variables in svf_eq provide slightly better numerical accuracy.
