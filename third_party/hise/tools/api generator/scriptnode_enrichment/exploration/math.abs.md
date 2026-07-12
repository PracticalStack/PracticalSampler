# math.abs - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:463`
**Base class:** `OpNodeBase<Operations::abs>` (via `OpNode<Operations::abs, 1>`)
**Classification:** audio_processor

## Signal Path

Computes the absolute value of each sample in the input signal, folding negative values to positive. The block path uses SIMD-accelerated `hmath::vabs(b)` (line 472). The frame path uses `hmath::abs(s)` per sample (line 481).

Unlike most mono-only math nodes that use the `OP_BLOCK2SINGLE` macro, abs has a custom block implementation that calls `hmath::vabs()` for vectorized processing, making it faster than per-sample iteration.

Formula: `output = |input|`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods receive the value argument but do not reference it.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The block path is SIMD-accelerated via `FloatVectorOperations::abs`. The frame path uses `hmath::abs()` which is implemented as `value * sign(value)`. Due to a quirk in the float overload of `hmath::sign`, `abs(0.0f)` produces negative zero (-0.0f), which is functionally equivalent to 0.0f in IEEE 754.
