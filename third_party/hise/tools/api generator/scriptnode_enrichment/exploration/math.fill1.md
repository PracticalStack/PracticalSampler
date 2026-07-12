# math.fill1 - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:107`
**Base class:** `OpNodeBase<Operations::fill1>` (via `OpNode<Operations::fill1, 1>`)
**Classification:** audio_processor

## Signal Path

Replaces the entire input signal with a constant DC value of 1.0. The block path uses SIMD-accelerated `hmath::vmovs(dst, 1.0)` (line 117). The frame path sets each sample to 1.0f directly (line 125).

The input signal is completely discarded. Useful for creating a DC signal for modulation or testing purposes.

Formula: `output = 1.0`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods do not reference the value argument.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
