# filters.ladder -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:170` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:510` (LadderSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<LadderSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a simple 4-pole (24 dB/oct) ladder lowpass filter. Four cascaded one-pole stages with feedback from stage 4 output.

Per-sample processing per channel (`processSample`):

```
resoclip = buf[3]                              // feedback from stage 4
in = input - resoclip * res                    // subtract feedback
buf[0] = (in      - buf[0]) * cut + buf[0]    // stage 1
buf[1] = (buf[0]  - buf[1]) * cut + buf[1]    // stage 2
buf[2] = (buf[1]  - buf[2]) * cut + buf[2]    // stage 3
buf[3] = (buf[2]  - buf[3]) * cut + buf[3]    // stage 4
output = 2.0 * buf[3]
```

4 state variables per channel (buf[0..3]).

## Gap Answers

### ladder-mode-values: Mode parameter range and values.

From `LadderSubType::getModes()`: `{ "LP24" }`. Only one mode. `setType(int /*t*/)` is empty -- mode changes are ignored. The Mode parameter range in the JSON (0-1, step 0) is anomalous; the filter only has one valid mode (LP24).

### ladder-vs-moog: Implementation difference.

See filters.moog exploration for detailed comparison. Summary:
- **Ladder:** Simpler algorithm, 4 state vars/channel, `cut = 2*pi*f/sr` (max 0.8), `res = Q/2` (0.3-4.0). More CPU-efficient.
- **Moog:** Stilson/Smith model, 8 state vars/channel, includes input history mixing (0.3 factor), specific analog-modeling constants (0.35013, 1.16). Warmer character.

Both are always 24 dB/oct lowpass. Choose moog for more "analog" character, ladder for simpler/lighter processing.

### ladder-slope: What is the filter slope?

24 dB/oct (4-pole). Four cascaded one-pole stages.

### ladder-gain-effect: Does Gain affect the ladder?

`LadderSubType::updateCoefficients()` has signature `(double sampleRate, double frequency, double q, double /*gain*/)` -- Gain is explicitly ignored. Q is used for resonance: `res = Q/2` clamped to 0.3-4.0.

### description-accuracy: Confirm characterisation.

Accurate description: "Simple 4-pole (24 dB/oct) ladder lowpass filter".

## Parameters

- **Frequency:** 20-20000 Hz. Cutoff frequency. Internally: `cut = 2*pi*f/sr` clamped to 0-0.8. Smoothed.
- **Q:** 0.3-9.9. Resonance. Internally: `res = Q/2` clamped to 0.3-4.0. Smoothed.
- **Gain:** -18 to +18 dB. **Ignored.**
- **Smoothing:** 0-1 seconds. Coefficient interpolation time.
- **Mode:** Single mode (LP24). Parameter exists but only one value is valid.
- **Enabled:** 0 or 1. Hard bypass.

## Polyphonic Behaviour

Same as all FilterNodeBase nodes.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

Very simple per-sample processing. 4 multiply-adds per sample per channel. Lightest of the multi-pole filters.
