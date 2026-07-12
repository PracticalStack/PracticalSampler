# math.sqrt - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:435`
**Base class:** `OpNode<Operations::sqrt, NV>` -> `OpNodeBase<Operations::sqrt>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Applies the square root function to each sample: `s = sqrtf(s)`. Block path uses `OP_BLOCK2SINGLE` (iterates channels, delegates to opSingle -- no SIMD acceleration). The Value parameter exists but is not used in processing.

## Gap Answers

### sqrt-negative-input: Does sqrt handle negative input values (e.g., from bipolar audio signals)? Is there any clamping or abs() applied before sqrtf?

No. The `opSingle` method at line 444 calls `sqrtf(s)` directly with no clamping or abs() guard. Negative input values will produce NaN (IEEE 754 behaviour for sqrt of negative numbers). Users must ensure non-negative input, for example by placing `math.abs` or `math.square` before `math.sqrt` in the signal chain. This node is primarily intended for unipolar modulation signals (0 to 1), not bipolar audio.

## Parameters

- **Value** (default 1.0): Not used in processing. The parameter exists due to the OpNode template but has no effect on the output.

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>` but is not read during processing.

## CPU Assessment

baseline: medium
polyphonic: true
scalingFactors: []

## Notes

NaN risk with negative input. No SIMD acceleration. The `value` argument is named `value` in the `opSingle` signature but is never referenced in the function body.
