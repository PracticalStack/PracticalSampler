# math.mul - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:63`
**Base class:** `OpNode<Operations::mul, NV>` -> `OpNodeBase<Operations::mul>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Multiplies every sample in the input signal by a scalar Value parameter. Block path uses SIMD-accelerated `hmath::vmuls`. Frame path uses per-sample `s *= value`.

## Parameters

- **Value** (default 1.0): Scalar multiplier applied to each sample. At 1.0 the signal passes through unchanged. At 0.0 the output is silence. Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent multiplier, typically driven by modulation. `prepare()` calls `value.prepare(ps)` to initialise voice count.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
