# math.rect - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:379`
**Base class:** `OpNodeBase<Operations::rect>` (via `OpNode<Operations::rect, 1>`)
**Classification:** audio_processor

## Signal Path

Converts a normalised signal into a binary gate by thresholding at 0.5. Uses the `OP_BLOCK2SINGLE` macro, so the block path iterates channels and delegates to `opSingle` per channel (no SIMD acceleration). The frame path computes `s = (float)(s >= 0.5f)` per sample (line 389).

The threshold is hardcoded at 0.5 and cannot be changed. Samples at or above 0.5 become 1.0; samples below 0.5 become 0.0.

Formula: `output = (input >= 0.5) ? 1.0 : 0.0`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods name the parameter `unused`.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The comparison uses `>=` so exactly 0.5 maps to 1.0. This is useful for converting a continuous modulation signal into a square-wave gate. For audio-rate use on a bipolar signal, consider using sig2mod first to shift the range to [0, 1] before applying rect.
