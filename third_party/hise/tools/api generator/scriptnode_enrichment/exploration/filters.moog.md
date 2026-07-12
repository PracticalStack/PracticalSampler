# filters.moog -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:168` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:268` (MoogFilterSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<MoogFilterSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a digital model of the Moog transistor ladder filter. The implementation is a Stilson/Smith approximation using four cascaded one-pole stages with feedback from the output of stage 4.

Per-sample processing per channel:

```
input -= out4 * fb                    // feedback from 4th stage
input *= 0.35013 * fss               // input scaling (fss = f^4)
out1 = input + 0.3*in1 + invF*out1   // stage 1
out2 = out1  + 0.3*in2 + invF*out2   // stage 2
out3 = out2  + 0.3*in3 + invF*out3   // stage 3
out4 = out3  + 0.3*in4 + invF*out4   // stage 4
output = 2.0 * out4                  // always from stage 4
```

8 state variables per channel (in1-in4, out1-out4). The filter always outputs from stage 4 (24 dB/oct) regardless of Mode setting.

## Gap Answers

### moog-mode-values: What do Mode values 0-2 correspond to?

From `MoogFilterSubType::getModes()`: `{ "One Pole", "Two Poles", "Four Poles" }`.

- 0 = One Pole (displayed as 6 dB/oct LP curve)
- 1 = Two Poles (displayed as 12 dB/oct resonant LP curve)
- 2 = Four Poles (displayed as 24 dB/oct ladder curve)

**However:** The Mode parameter only affects the filter display curve (via `getCoefficientTypeList()`), NOT the actual audio processing. The `setType(int /*t*/)` method is empty. Processing always runs the full 4-pole ladder and outputs from stage 4. See issues.md.

### moog-vs-ladder: What is the difference between moog and ladder?

Both are 4-pole (24 dB/oct) lowpass ladder filters, but they use different algorithms:

**filters.moog (MoogFilterSubType):** Stilson/Smith Moog approximation. Uses 8 state variables per channel (4 input history + 4 output history). Coefficients: `fc = freq / (0.5*sr)`, `f = fc * 1.16`, `fb = res * (1.0 - 0.15*f*f)`. Has a warmer, more "classic" character due to the 0.3 input history mixing and the specific scaling factors (0.35013, 1.16).

**filters.ladder (LadderSubType):** Simpler 4-pole implementation. Uses 4 state variables per channel (one buffer per stage). Coefficients: `cut = 2*pi*freq/sr` (clamped to 0.8), `res = Q/2` (clamped to 0.3-4.0). More CPU-efficient but less "analog" character.

Key differences:
- Moog has 3 display modes (but processing is always 4-pole); Ladder has 1 mode (LP24)
- Moog has more state variables (8 vs 4 per channel)
- Moog's coefficient formula includes specific analog-modeling constants
- Both ignore Q and Gain parameters for processing

### moog-self-oscillation: Does Q cause self-oscillation?

Q is mapped to resonance feedback: `res = q / 2.0` clamped to max 4.0, then `fb = res * (1 - 0.15*f*f)`. At Q=9.9, res=4.0. With fb=4*(1-0.15*f*f), the feedback can approach self-oscillation at low frequencies (where f*f is small). There is no explicit nonlinear saturation to prevent runaway amplitude, but the 0.35013 input scaling and the feedback formula inherently limit the gain.

### moog-gain-effect: Does Gain affect the Moog filter?

`MoogFilterSubType::updateCoefficients()` has signature `(double sampleRate, double frequency, double q, double /*gain*/)` -- Gain is explicitly ignored. Q is used for resonance feedback.

### description-accuracy: Confirm characterisation.

Accurate description: "Moog-style transistor ladder lowpass filter (24 dB/oct, Stilson/Smith model)". Always 4-pole despite Mode parameter suggesting otherwise.

## Parameters

- **Frequency:** 20-20000 Hz. Cutoff frequency. Smoothed.
- **Q:** 0.3-9.9. Controls resonance feedback (res = Q/2, max 4.0). Higher values increase resonance toward self-oscillation.
- **Gain:** -18 to +18 dB. **Ignored.**
- **Smoothing:** 0-1 seconds. Coefficient interpolation time.
- **Mode:** 0-2 integer. **Display-only** -- changes filter curve visualization but not audio processing. See issues.md.
- **Enabled:** 0 or 1. Hard bypass.

## Polyphonic Behaviour

Same as all FilterNodeBase nodes.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Per-sample processing with simple arithmetic. 8 state variables per channel is more than SVF (3) but still lightweight.
