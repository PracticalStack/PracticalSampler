# math.sin - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:337`
**Base class:** `OpNodeBase<Operations::sin>` (via `OpNode<Operations::sin, 1>`)
**Classification:** audio_processor

## Signal Path

Applies the sine function to each sample of the input signal. Uses the `OP_BLOCK2SINGLE` macro, so the block path iterates channels and delegates to `opSingle` per channel (no SIMD acceleration). The frame path computes `s = sinf(s)` per sample (line 347).

The input is interpreted as radians. Typically preceded by math.pi (with Value=2.0) to convert a normalised [0, 1] ramp into a full sine cycle.

Formula: `output = sin(input)`

## Parameters

- **Value** (default 2.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods name the parameter `unused`. The default of 2.0 is inherited from `getDefaultValue()` but has no effect.

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: []

## Notes

The `sinf()` call is a transcendental function, making this node more expensive than simple arithmetic math nodes. No fast approximation is used. For lower-cost alternatives, consider using a table-based lookup via math.table with a pre-computed sine table.
