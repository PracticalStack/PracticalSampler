# math.sig2mod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:351`
**Base class:** `OpNodeBase<Operations::sig2mod>` (via `OpNode<Operations::sig2mod, 1>`)
**Classification:** audio_processor

## Signal Path

Converts a bipolar audio signal [-1, 1] to a unipolar modulation signal [0, 1]. Uses the `OP_BLOCK2SINGLE` macro, so the block path iterates channels and delegates to `opSingle` per channel (no SIMD acceleration). The frame path computes `s = s * 0.5f + 0.5f` per sample (line 360).

Formula: `output = input * 0.5 + 0.5`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods name the parameter `unused`.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

Inverse of math.mod2sig. Together they convert between bipolar audio range [-1, 1] and unipolar modulation range [0, 1]. Mapping: -1.0 -> 0.0, 0.0 -> 0.5, 1.0 -> 1.0.
