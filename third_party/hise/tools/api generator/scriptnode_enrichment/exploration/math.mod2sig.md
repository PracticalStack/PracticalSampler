# math.mod2sig - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:365`
**Base class:** `OpNodeBase<Operations::mod2sig>` (via `OpNode<Operations::mod2sig, 1>`)
**Classification:** audio_processor

## Signal Path

Converts a unipolar modulation signal [0, 1] to a bipolar audio signal [-1, 1]. Uses the `OP_BLOCK2SINGLE` macro, so the block path iterates channels and delegates to `opSingle` per channel (no SIMD acceleration). The frame path computes `s = s * 2.0f - 1.0f` per sample (line 374).

Formula: `output = input * 2.0 - 1.0`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods name the parameter `unused`.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

Inverse of math.sig2mod. Together they convert between unipolar modulation range [0, 1] and bipolar audio range [-1, 1]. Mapping: 0.0 -> -1.0, 0.5 -> 0.0, 1.0 -> 1.0.
