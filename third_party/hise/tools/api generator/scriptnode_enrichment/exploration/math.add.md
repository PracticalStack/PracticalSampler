# math.add - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:85`
**Base class:** `OpNode<Operations::add, NV>` -> `OpNodeBase<Operations::add>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Adds a scalar Value to every sample in the input signal. Block path uses SIMD-accelerated `hmath::vadds`. Frame path uses per-sample `s += value`. Effectively applies a DC offset.

## Parameters

- **Value** (default 0.0): DC offset added to each sample. At 0.0 the signal passes through unchanged. Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent offset value.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
