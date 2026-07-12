# container.frame2_block -- C++ Exploration (Variant)

**Base variant:** container.frame1_block
**Variant parameter:** NumChannels = 2 (stereo)

## Variant-Specific Behaviour

Uses `wrap::frame<2, T>` = `wrap::fix<2, wrap::frame_x<T>>` instead of
`wrap::frame<1, T>`. Children receive `span<float, 2>` containing [left, right]
for each sample.

Key differences from frame1_block:
- `fix<2>` casts to `ProcessData<2>` and processes both stereo channels
- `FrameConverters::processFix<2>()` interleaves both channels into each frame
- Children see `span<float, 2>&` in processFrame() with frame[0]=left, frame[1]=right
- blockSize=1, numChannels=2 in PrepareSpecs

The interleaving mechanism is the same `FrameProcessor<2>` from
snex_FrameProcessor.h. Each `next()` call loads one sample from each of the
2 channel pointers into the frame span, processes, then writes back.

**Leftover buffer handling:** If the network has more than 2 channels,
`SingleSampleBlock<2>::prepare()` allocates a leftover buffer for the extra
channels. The `process()` method builds a channel pointer array where the
first 2 pointers come from the input data and the remaining point to the
zeroed leftover buffer. This means extra channels are zeroed, not passed through.

This is the most commonly used frame container since most scriptnode networks
are stereo.

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: []

Slightly more overhead than frame1_block due to interleaving 2 channels
per sample instead of 1. Still dominated by per-sample call overhead.
