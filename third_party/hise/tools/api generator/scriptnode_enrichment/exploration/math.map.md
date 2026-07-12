# math.map - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:599-691`
**Base class:** `mothernode`
**Classification:** audio_processor

## Signal Path

Linear range mapper. For each sample:
1. Subtract InputStart from the input value.
2. Clamp the result to [0, abs(InputEnd - InputStart)].
3. Multiply by rangeFactor (precomputed as (OutputEnd - OutputStart) / (InputEnd - InputStart)).
4. Add OutputStart.

The operation applies to all channels via the `op()` template method. Both block and frame paths call the same `op()` method, so behaviour is identical regardless of processing context.

## Gap Answers

### input-clamping-behaviour: How does math.map handle input values outside the [InputStart, InputEnd] range?

The input is clamped after subtracting InputStart. Specifically, after `value -= inStart`, the result is clamped to `[0, clipMax]` where `clipMax = abs(inEnd - inStart)`. This means input values below InputStart are clamped to OutputStart, and values above InputEnd are clamped to OutputEnd. No extrapolation occurs. The clamping uses `hmath::range(value, 0.0f, clipMax)`.

### mapping-linearity: Is the mapping purely linear?

Yes. The mapping is strictly linear: subtract, clamp, multiply by a constant factor, add offset. There is no skew, curve, or non-linear shaping. The rangeFactor is a simple ratio `(OutputEnd - OutputStart) / (InputEnd - InputStart)`.

### processing-granularity: Does math.map process per-sample or per-block with SIMD?

The `process()` method iterates channels and calls `op(b)` on each channel block. The `op()` method on a block calls `hmath::range(value, 0.0f, clipMax)` which uses SIMD-accelerated `FloatVectorOperations::clip` for the clamp step. The subtract, multiply, and add steps use `dyn<float>` arithmetic operators which are also SIMD-accelerated via `FloatVectorOperations`. So the block path is fully SIMD-accelerated.

The `processFrame()` method iterates samples and calls `op(s)` per sample (scalar path).

### inverted-range-behaviour: What happens when InputStart > InputEnd or OutputStart > OutputEnd?

Inverted ranges work correctly. When InputStart > InputEnd, `inEnd - inStart` is negative, so `inLengthInv` is negative, and `rangeFactor` flips sign accordingly. The `clipMax` uses `hmath::abs(inEnd - inStart)` ensuring the clamp range is always non-negative. When InputStart equals InputEnd, `inLengthInv` is set to 0 (division-by-zero guard), producing a constant output of OutputStart.

## Parameters

- **InputStart** (default 0.0, range [0,1]): Start of expected input range. Subtracted from input before mapping.
- **InputEnd** (default 1.0, range [0,1]): End of expected input range. Defines clamp boundary together with InputStart.
- **OutputStart** (default 0.0, range [0,1]): Start of output range. Added as offset after scaling.
- **OutputEnd** (default 1.0, range [0,1]): End of output range. Defines output scale together with OutputStart.

All four parameters trigger a recalculation of `rangeFactor` and `clipMax` in `setParameter()`.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
