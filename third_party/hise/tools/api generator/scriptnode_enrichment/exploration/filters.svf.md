# filters.svf -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:162` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:626` (StateVariableFilterSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<StateVariableFilterSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a second-order state variable filter (SVF). The SVF topology maintains two state variables per channel (`z1_A`, `v2`) plus previous input (`v0z`). Depending on the Mode, different combinations of the state variables form the output:

- **LP (mode 0):** output = v2 (integrator 2 output)
- **HP (mode 1):** output = input - k * z1_A - v2
- **BP (mode 2):** output = z1_A (integrator 1 output)
- **Notch (mode 3):** output = input - k * z1_A
- **Allpass (mode 4):** Uses a separate Zavalishin TPT formulation: output = input - 4*R*BP

For modes 0-3, the SVF uses a trapezoidal integration scheme with coefficients g1-g4 derived from `tan(pi * freq / sampleRate)` and a damping factor `k = 1 - 0.99 * scaledQ` where scaledQ = Q * 0.1 clamped to 0..9.999.

For Allpass (mode 4), a different coefficient path is used with bilinear-transform pre-warping. Coefficients gCoeff, RCoeff, x1, x2 are computed via Zavalishin's formulation with R = 1/(2*Q).

Processing is per-sample within the block (inner loop over samples, outer loop over channels).

## Gap Answers

### filter-base-architecture: How does filter_base process/processFrame work?

`FilterNodeBase::process()` converts to AudioSampleBuffer, creates `FilterHelpers::RenderData`, and calls `filter.get().render(r)` on the current voice's `MultiChannelFilter`. `render()` calls `update(r)` to advance smoothed parameter values, then calls `internalFilter.processSamples()`.

`FilterNodeBase::processFrame()` calls `filter.get().processFrame(data.begin(), data.size())`. Inside `MultiChannelFilter::processFrame()`, a frame counter decrements; every 64 frames it calls `updateEvery64Frame()` which advances smoothed values and calls `internalFilter.updateCoefficients()` if dirty. Then `internalFilter.processFrame()` is called.

Coefficient smoothing: `MultiChannelFilter` uses `juce::LinearSmoothedValue<double>` for frequency, Q, and gain. The smoothing rate is `sampleRate / 64.0` (i.e., coefficients update once per 64 samples in frame mode, once per block in block mode). Smoothing time is set in seconds via the Smoothing parameter.

### svf-mode-values: What do Mode values 0-4 correspond to?

From `StateVariableFilterSubType::getModes()`: `{ "LP", "HP", "BP", "Notch", "Allpass" }`.

- 0 = LP (Low Pass with resonance)
- 1 = HP (High Pass)
- 2 = BP (Band Pass)
- 3 = Notch (Band Reject)
- 4 = Allpass

### svf-topology: What SVF implementation is used?

Modes 0-3 use a trapezoidal-integration SVF derived from Andrew Simper's approach. Three state variables per channel: `v0z` (previous input), `z1_A` (integrator 1 state), `v2` (integrator 2 state). Plus shared coefficients `k, g1, g2, g3, g4`.

Mode 4 (Allpass) uses Vadim Zavalishin's TPT (topology-preserving transform) SVF with bilinear pre-warping. Coefficients: `gCoeff` (integrator gain), `RCoeff` (damping = 1/(2Q)), `x1`, `x2`.

### smoothing-units: What unit does the Smoothing parameter represent?

Smoothing is passed directly to `MultiChannelFilter::setSmoothingTime(double)` which stores it as `smoothingTimeSeconds`. Range is 0 to 1 (seconds). Default 0.01 = 10ms. Skew 0.301 gives logarithmic feel. The smoothing applies to all three response parameters (Frequency, Q, Gain) via `LinearSmoothedValue` at an update rate of sampleRate/64.

### gain-parameter-relevance: For which modes does Gain affect the response?

`StateVariableFilterSubType::updateCoefficients()` has signature `(double sampleRate, double frequency, double q, double /*gain*/)` -- the gain parameter is explicitly unnamed/ignored. Gain has **no effect** on any SVF mode. The Gain parameter exists only for interface consistency with FilterNodeBase's shared parameter set.

### description-accuracy: Confirm the SVF characterisation.

The base description "A filter node" is generic. Accurate description: "State variable filter with LP, HP, BP, Notch, and Allpass modes". Second-order (12 dB/oct for LP/HP), per-sample processing, uses trapezoidal integration (modes 0-3) or Zavalishin TPT (allpass mode).

### enabled-bypass-behaviour: What happens when Enabled=0?

`FilterNodeBase::setEnabled(double isEnabled)` sets `enabled = isEnabled > 0.5`. In `process()` and `processFrame()`, the filter body is wrapped in `if(enabled)`. When disabled, processing is skipped entirely -- audio passes through unmodified. There is no crossfade; toggling Enabled mid-block produces an instantaneous switch.

## Parameters

- **Frequency:** 20-20000 Hz (skew for center 1000). Sets cutoff/center frequency. Smoothed.
- **Q:** 0.3-9.9 (skew for center 1.0). Controls resonance. Internally scaled: scaledQ = Q * 0.1 (range 0.03-0.99). For LP/HP/BP/Notch: k = 1 - 0.99*scaledQ (higher Q = lower k = more resonance). For Allpass: R = 1/(2*Q). Smoothed.
- **Gain:** -18 to +18 dB. Converted to linear via `Decibels::decibelsToGain()` before being passed to MultiChannelFilter. **Ignored** by SVF -- has no effect on any mode.
- **Smoothing:** 0-1 seconds (skew for center 0.1). Controls interpolation time for Frequency/Q/Gain changes.
- **Mode:** 0-4 integer. Selects filter type (LP/HP/BP/Notch/Allpass). Changes processing path in processSamples/processFrame.
- **Enabled:** 0 or 1. Hard bypass when 0 (no processing, no crossfade).

## Conditional Behaviour

Mode 0-3 share the same coefficient computation path (trapezoidal integration) but extract different outputs from the state variables.

Mode 4 (Allpass) uses a completely different coefficient computation path (Zavalishin TPT with bilinear pre-warping).

**Note:** There is an inconsistency between `processSamples()` and `processFrame()` for Allpass mode. In processSamples (block path), HP is computed as `(input - x1*z1 - v2) * x2` (multiply). In processFrame, HP is computed as `(input - x1*z1 - v2) / x2` (divide). These produce different results since x2 != 1/x2. See issues.md.

## Polyphonic Behaviour

`PolyData<FilterObject, NumVoices> filter` stores one `MultiChannelFilter<StateVariableFilterSubType>` per voice. Each voice has independent state variables and independent smoothed parameter values. Parameter setters iterate all voices (or current voice if modulated per-voice).

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Per-sample processing with simple arithmetic (no transcendentals in the processing loop -- tan() is called only during coefficient updates). Coefficient updates happen once per block or every 64 frames.

## Notes

The SVF is the recommended general-purpose filter due to its stability under modulation (trapezoidal integration avoids the instability issues of Direct Form biquads). The Q scaling (0.3-9.9 mapped to 0.03-0.99 internal) prevents self-oscillation at maximum Q.
