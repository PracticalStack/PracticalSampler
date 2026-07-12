# math.sub - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:225`
**Base class:** `OpNode<Operations::sub, NV>` -> `OpNodeBase<Operations::sub>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Subtracts a scalar Value from every sample in the input signal. Block path uses `hmath::vadds(b, -value)` (SIMD-accelerated addition of the negated value). Frame path uses per-sample `s -= value`.

## Parameters

- **Value** (default 0.0): Scalar subtracted from each sample. At 0.0 the signal passes through unchanged. Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent subtraction value.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
