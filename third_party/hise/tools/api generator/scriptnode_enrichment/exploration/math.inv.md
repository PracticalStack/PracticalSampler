# math.inv - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:202`
**Base class:** `OpNodeBase<Operations::inv>` (via `OpNode<Operations::inv, 1>`)
**Classification:** audio_processor

## Signal Path

Negates (inverts the phase of) the input signal by multiplying every sample by -1.0. The block path uses SIMD-accelerated `hmath::vmuls(b, -1.0f)` (line 211). The frame path multiplies each sample by -1.0f (line 219).

Formula: `output = -input`

## Parameters

- **Value** (default 0.0): Present in the UI but ignored in processing. The `op()` and `opSingle()` methods do not reference the value argument.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

This performs bipolar phase inversion (negation), distinct from math.mod_inv which performs unipolar inversion (1.0 - s). For audio signals in the [-1, 1] range, inv flips the waveform vertically around zero. For modulation signals in [0, 1], use mod_inv instead.
