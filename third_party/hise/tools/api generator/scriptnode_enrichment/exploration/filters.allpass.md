# filters.allpass -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:166` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:451` (PhaseAllpassSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<PhaseAllpassSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a 6-stage cascaded first-order allpass filter chain with feedback. This is a phaser-style allpass, NOT a standard second-order allpass filter.

Per-sample processing (`InternalFilter::getNextSample`):
1. Compute delay coefficient from `minDelay` (derived from frequency)
2. Set all 6 allpass delay stages to the same coefficient
3. Process input + feedback through 6 cascaded `AllpassDelay` stages
4. Store output as `currentValue` for feedback
5. Return `input + output` (mix of dry input and allpass-processed signal)

Each `AllpassDelay` stage is a first-order allpass: `y = input * (-delay) + currentValue; currentValue = y * delay + input`. The 6 cascaded stages produce a total phase shift of up to 6 * 180 = 1080 degrees.

The feedback path (`currentValue * feedback`) creates resonant peaks/notches characteristic of phaser effects.

## Gap Answers

### allpass-mode-values: Mode parameter.

From `PhaseAllpassSubType::getModes()`: `{ "All Pass" }`. Only one mode. `setType(int /**/)` is empty. The Mode parameter range in the JSON (0-1, step 0) is anomalous; the filter only has one valid mode.

### allpass-phase-behaviour: What is the phase response?

This is a 6-stage cascaded first-order allpass with feedback. Each stage introduces up to 180 degrees of phase shift. The total phase shift depends on frequency relative to the center frequency (set by the Frequency parameter). At the center frequency, maximum phase shift occurs, creating the characteristic phaser notches when mixed with the dry signal (which is done internally: output = input + allpass_chain_output).

### allpass-q-effect: How does Q affect the allpass?

Q controls the feedback amount: `feedback = jmap(q, 0.3, 9.9, 0.0, 0.99)`. Higher Q values increase feedback, creating sharper resonant peaks at the notch frequencies. At Q=0.3 (minimum), feedback=0 and the effect is a pure allpass chain with no resonance. At Q=9.9 (maximum), feedback=0.99 which creates strong resonant peaks (near self-oscillation).

### allpass-gain-effect: Does Gain affect the allpass?

`PhaseAllpassSubType::updateCoefficients()` has signature `(double sampleRate, double frequency, double q, double /*gain*/)` -- Gain is explicitly ignored.

### description-accuracy: Confirm characterisation.

Accurate description: "Six-stage cascaded allpass filter with feedback (phaser-style phase shifting effect)". This is NOT a simple second-order allpass for phase correction -- it is a multi-stage phaser effect. The output includes the dry signal mixed with the allpass chain output.

## Parameters

- **Frequency:** 20-20000 Hz. Sets the center frequency for the allpass delay. Internally mapped to `minDelay = frequency / (sampleRate/2)`. Smoothed.
- **Q:** 0.3-9.9. Controls feedback amount (0.0 to 0.99). Higher Q = more resonance/phaser depth. Smoothed.
- **Gain:** -18 to +18 dB. **Ignored.**
- **Smoothing:** 0-1 seconds. Coefficient interpolation time.
- **Mode:** Single mode (AllPass). Parameter exists but only one value is valid.
- **Enabled:** 0 or 1. Hard bypass.

## Polyphonic Behaviour

Same as all FilterNodeBase nodes. One `InternalFilter` (with 6 AllpassDelay stages) per channel per voice.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Per-sample processing through 6 cascaded allpass stages. Each stage is 2 multiply-adds. Total: 12 multiply-adds + 1 feedback multiplication per sample per channel.

## Notes

This filter is misnamed from a DSP perspective. A standard "allpass filter" passes all frequencies at equal amplitude with only phase shift. This node is actually a multi-stage phaser effect that produces amplitude notches (because it mixes dry + wet). The name "allpass" refers to the individual filter stages being allpass, not to the overall frequency response.

For a true second-order allpass (phase correction without amplitude change), use filters.svf in Allpass mode (mode 4).
