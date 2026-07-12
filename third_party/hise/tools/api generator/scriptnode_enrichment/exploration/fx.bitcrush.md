# fx.bitcrush -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FXNodes.h:177` (class), `FXNodes.h:151` (getBitcrushedValue helper)
**Base class:** `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is quantised to a reduced bit depth. The free function `getBitcrushedValue()` does the work:

```
invStepSize = pow(2, bitDepth)
stepSize = 1.0 / invStepSize
```

Two modes control the quantisation rounding:

- **DC Offset mode (bipolar=false):** `output = stepSize * ceil(input * invStepSize) - 0.5 * stepSize`
  Applies ceil rounding then shifts down by half a step. This centres the quantisation steps symmetrically around zero but introduces a DC offset of -0.5*stepSize.

- **Bipolar mode (bipolar=true):** For positive samples: `output = stepSize * floor(input * invStepSize)`. For negative samples: `output = stepSize * ceil(input * invStepSize)`. This rounds toward zero for both polarities, producing symmetric distortion without DC offset.

Processing is per-block (channel iteration) with a per-sample fallback via processFrame. The `process()` method iterates channels and calls `getBitcrushedValue()` on each channel block. `processFrame()` calls `getBitcrushedValue()` on the frame directly.

At BitDepth=16: invStepSize=65536, stepSize=~0.0000153. Quantisation noise is below audible threshold -- effectively transparent.
At BitDepth=4: invStepSize=16, stepSize=0.0625. Heavy quantisation with 16 discrete levels.

Fractional bit depth is fully supported -- pow(2, bitDepth) produces continuous values for non-integer inputs.

## Gap Answers

### processing-order: Does process() iterate per-sample or per-block?

Per-block. `process()` iterates channels via `for (auto ch : d)`, converts each to a block, and passes the block to `getBitcrushedValue()` which iterates samples internally. Falls back to `processFrame()` only when called in frame context. No frame conversion overhead in block mode.

### bitdepth-quantisation: Is fractional bit depth supported? Is 16 bits transparent?

Yes, fractional bit depth is fully supported. `hmath::pow(2.0f, bitDepth)` produces continuous values. At 16.0 bits the step size is ~1.5e-5, well below audible threshold -- effectively transparent. The step of 0.1 in the parameter range enables smooth modulation of bit depth.

### mode-values: Confirm DC Offset vs Bipolar modes

Confirmed. Mode parameter uses `setParameterValueNames({"DC", "Bipolar"})`. Mode 0 ("DC") sets `bipolar=false`: uses ceil rounding with a half-step shift, producing a small DC offset. Mode 1 ("Bipolar") sets `bipolar=true`: rounds toward zero (floor for positive, ceil for negative), symmetric around zero with no DC offset. The existing doc's description of ceil vs floor is confirmed by the C++ source.

### polyphonic-state: Per-voice state?

Yes. `PolyData<float, NumVoices> bitDepth` stores per-voice bit depth. Each voice can have an independently modulated bit depth value. The `bipolar` flag is NOT per-voice -- it is a plain `bool`, shared across all voices. This means Mode changes affect all voices simultaneously, but BitDepth can differ per voice.

## Parameters

- **BitDepth (enum index 0):** Range 4.0-16.0, step 0.1, default 16.0. Setter `setBitDepth()` clamps to 1.0-16.0 (note: lower clamp in setter is 1.0, not 4.0 -- the parameter range is 4-16 but the setter allows down to 1.0 if driven directly). Stored in `PolyData<float, NumVoices>`.
- **Mode (enum index 1):** Range 0-1, step 1, default 0. Named values "DC" (0) and "Bipolar" (1). Setter `setMode()` uses threshold `> 0.5`. Stored as plain `bool bipolar`, NOT per-voice.

## Polyphonic Behaviour

BitDepth is per-voice via `PolyData<float, NumVoices>`. Each voice maintains its own quantisation depth, enabling per-voice modulation. Mode (bipolar flag) is shared across all voices -- a single `bool` member, not wrapped in PolyData.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Processing is simple arithmetic (pow, floor/ceil, multiply) per sample. No state between samples, no feedback, no transcendental functions beyond the pow() in the quantisation step (which is recomputed from the smoothed parameter). Very efficient.

## Notes

The `getBitcrushedValue()` helper is a free function template defined at file scope (FXNodes.h:151), not a member. It operates on any iterable container of floats. The bitDepth member uses PolyData but is a plain float, not sfloat/sdouble -- there is no built-in parameter smoothing. Rapid BitDepth changes may produce zipper artifacts.
