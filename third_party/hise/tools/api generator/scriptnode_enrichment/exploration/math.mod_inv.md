# math.mod_inv - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:180`
**Base class:** `OpNodeBase<Operations::mod_inv>` (via `OpNode<Operations::mod_inv, 1>`)
**Classification:** audio_processor

## Signal Path

Inverts a unipolar modulation signal within the 0-1 range. The block path uses two SIMD operations: `hmath::vmuls(b, -1.0f)` followed by `hmath::vadds(b, 1.0f)` (lines 190-191). The frame path computes `s = 1.0f - s` per sample (line 198).

Formula: `output = 1.0 - input`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods do not reference the value argument.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

Distinct from math.inv which negates the signal (-s). mod_inv is designed for unipolar modulation signals in [0, 1]: an input of 0.0 becomes 1.0, and 1.0 becomes 0.0. If applied to a bipolar signal in [-1, 1], the output range shifts to [0, 2].
