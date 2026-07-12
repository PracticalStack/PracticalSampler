# math.square - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:414`
**Base class:** `OpNode<Operations::square, NV>` -> `OpNodeBase<Operations::square>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Multiplies each sample by itself: `s *= s` (equivalently `s = s * s`). Block path uses SIMD-accelerated `hmath::vmul(b, b)`. Frame path uses per-sample `s *= s`. The Value parameter exists (part of the OpNode template) but is not used in the operation.

Output is always non-negative for real input. This acts as a simple waveshaper that compresses the dynamic range and doubles the frequency of the input signal.

## Parameters

- **Value** (default 1.0): Not used in processing. The parameter exists due to the OpNode template but has no effect on the output.

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>` but is not read during processing.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

Despite Value being unused, the node is defined with `DEFINE_OP_NODE` (polyphonic). The self-multiplication via `vmul(b, b)` is SIMD-accelerated and efficient.
