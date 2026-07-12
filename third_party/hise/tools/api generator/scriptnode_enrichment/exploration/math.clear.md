# math.clear - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:129`
**Base class:** `OpNodeBase<Operations::clear>` (via `OpNode<Operations::clear, 1>`)
**Classification:** audio_processor

## Signal Path

Replaces the entire input signal with silence (zero). The block path uses SIMD-accelerated `hmath::vmovs(dst, 0.0f)` (line 139). The frame path sets each sample to 0.0f directly (line 147).

The input signal is completely discarded. This node is useful as a signal reset point in container chains.

Formula: `output = 0.0`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods do not reference the value argument.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
