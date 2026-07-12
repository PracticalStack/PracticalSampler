# math.tanh - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:302`
**Base class:** `OpNode<Operations::tanh, NV>` -> `OpNodeBase<Operations::tanh>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Applies hyperbolic tangent saturation to the input signal. Each sample is computed as `s = tanhf(s * value)`. The Value parameter scales the input before tanh is applied, controlling the saturation amount. Block path uses `OP_BLOCK2SINGLE` (iterates channels, delegates to opSingle per channel -- no SIMD acceleration).

At Value=1.0, gentle soft clipping. Higher values drive the signal harder into the tanh curve, producing more pronounced saturation. Output is always bounded to (-1, 1) regardless of input amplitude.

## Parameters

- **Value** (default 1.0): Input drive/saturation amount. Scales the signal before tanh is applied. Higher values produce more saturation. Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent saturation amount.

## CPU Assessment

baseline: medium
polyphonic: true
scalingFactors: []

## Notes

Uses `tanhf` (C standard library) per sample with no SIMD acceleration. The per-sample transcendental function call makes this more expensive than simple arithmetic math nodes.
