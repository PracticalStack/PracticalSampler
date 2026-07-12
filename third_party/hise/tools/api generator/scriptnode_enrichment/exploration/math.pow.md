# math.pow - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:449`
**Base class:** `OpNode<Operations::pow, NV>` -> `OpNodeBase<Operations::pow>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Raises each sample to the power of the Value parameter: `s = powf(s, value)`. Block path uses `OP_BLOCK2SINGLE` (iterates channels, delegates to opSingle -- no SIMD acceleration). Frame path applies `powf` per sample.

At Value=1.0 (default), the signal passes through unchanged. Values less than 1.0 compress the dynamic range (concave curve for positive input). Values greater than 1.0 expand the dynamic range (convex curve). Negative base values with non-integer exponents will produce NaN.

## Parameters

- **Value** (default 1.0): Exponent applied to each sample via `powf(s, value)`. Controls the curve shape. Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent exponent.

## CPU Assessment

baseline: medium
polyphonic: true
scalingFactors: []

## Notes

Uses `powf` (C standard library) per sample with no SIMD acceleration. Like `math.sqrt`, negative input values with fractional exponents produce NaN. Best suited for unipolar (0 to 1) modulation signal shaping.
