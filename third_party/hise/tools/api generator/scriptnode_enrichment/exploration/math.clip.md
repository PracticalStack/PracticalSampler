# math.clip - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:393`
**Base class:** `OpNode<Operations::clip, NV>` -> `OpNodeBase<Operations::clip>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Clamps every sample to the symmetric range [-Value, Value]. Block path uses `hmath::vclip(b, -value, value)` (SIMD-accelerated). Frame path contains a bug: it computes `s *= jlimit(-value, value, s)` instead of `s = jlimit(-value, value, s)`, multiplying the sample by its own clamped value rather than simply clamping.

## Gap Answers

### clip-opsingle-bug-impact: The opSingle path multiplies s by the clamped value instead of just clamping. In what container configurations does the frame path get used vs the block path?

The block path (`op()`) is used in normal `container.chain` and any block-based container. The frame path (`opSingle()`) is used when the node is inside a frame-processing container such as `container.frame2_block`, `container.frame1_block`, or `container.framex_block`. In those frame-based contexts, `processFrame()` is called instead of `process()`, which delegates to `opSingle()`. Users placing `math.clip` inside frame containers will get incorrect behaviour: the output becomes `s * clamp(s, -v, v)` rather than `clamp(s, -v, v)`.

## Parameters

- **Value** (default 1.0): Symmetric clipping limit. Signal is clamped to [-Value, Value]. At 1.0 no clipping occurs for signals in the normal [-1, 1] range. Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent clipping threshold.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

The block path is correct and SIMD-accelerated. The frame path bug means behaviour differs depending on container context. For block-based containers (the common case), the node works as expected.
