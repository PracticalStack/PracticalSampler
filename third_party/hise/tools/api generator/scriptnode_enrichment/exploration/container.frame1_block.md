# container.frame1_block -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/processors.h:638` (wrap::frame alias), `hi_dsp_library/node_api/nodes/processors.h:592` (wrap::frame_x), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:957` (SingleSampleBlock)
**Base class:** `SerialNode` (interpreted)
**Classification:** container

## Signal Path

Serial container that converts block-based processing to per-sample (frame)
processing with a fixed channel count of 1 (mono). Children receive one sample
at a time as a `span<float, 1>` frame, with blockSize=1 in their PrepareSpecs.

```
Input block (N samples, C channels)
  -> fix<1>: cast to ProcessData<1> (only channel 0)
    -> frame_x: for each sample:
      -> children.processFrame(span<float, 1>{sample})
  -> Output (channel 0 processed, extra channels handled by leftover buffer)
```

The wrapper composition is `wrap::frame<1, T>` which expands to
`wrap::fix<1, wrap::frame_x<T>>` (processors.h:638).

## Gap Answers

### frame-processing-mechanism: How mono frame processing works

`wrap::fix<1, wrap::frame_x<T>>` composes two wrappers:

1. **fix<1>** (processors.h:248-331): Overrides `ps.numChannels = 1` in prepare().
   In process(), casts incoming data to `ProcessData<1>` via `data.as<ProcessData<1>>()`.
   In processFrame(), casts to `span<float, 1>`.

2. **frame_x** (processors.h:592-635): Sets `ps.blockSize = 1` in prepare().
   In process(), since the data has compile-time channel count 1 (from fix<1>),
   calls `FrameConverters::processFix<1>(&obj, data)`.

`FrameConverters::processFix<1>()` (snex_ProcessDataTypes.h:578-589):
```cpp
auto& fixData = data.template as<ProcessData<1>>();
auto fd = fixData.toFrameData();
while (fd.next())
    obj.processFrame(fd.toSpan());
```

This creates a `FrameProcessor<1>` which iterates sample-by-sample. Each call
to `next()` loads one sample from channel 0 into a `span<float, 1>`, calls
`processFrame()`, then writes the result back. Children receive blockSize=1
in PrepareSpecs and see `span<float, 1>&` in processFrame().

### extra-channel-handling: What happens to channels beyond channel 0

The interpreted `SingleSampleBlock<1>::process()` (NodeContainerTypes.h:993-1029)
handles channel mismatch explicitly:

```cpp
int numLeftOverChannels = NumChannels - data.getNumChannels();
if (numLeftOverChannels > 0)
{
    leftoverBuffer.clear();
    for (int i = 0; i < numLeftOverChannels; i++)
        channels[data.getNumChannels() + i] = leftoverBuffer.getWritePointer(i);
}
```

Wait -- for frame1_block, `NumChannels = 1`. If the network has 2 channels,
`numLeftOverChannels = 1 - 2 = -1`, which is <= 0. So the leftover buffer
is NOT used in this direction.

The key is that `fix<1>` in the compiled path simply processes only channel 0
and ignores channel 1. The extra channels pass through unmodified (the original
channel pointers are not touched). So in a stereo network:
- Channel 0: processed by the frame1_block children
- Channel 1: passes through unmodified

The `prepare()` (NodeContainerTypes.h:980-991) allocates a leftover buffer only
when `NumChannels > ps.numChannels` (i.e., the container wants MORE channels
than available). For frame1_block (NumChannels=1), this condition is false in
any network with >= 1 channel, so no leftover buffer is allocated.

### bypass-frame-revert: Bypass behavior

When bypassed, frame1_block reverts to block processing. `SingleSampleBlock::setBypassed()`
(NodeContainerTypes.h:1040-1055):
```cpp
SerialNode::setBypassed(shouldBeBypassed);
PrepareSpecs ps;
ps.blockSize = originalBlockSize;
// ...
prepare(ps);
getRootNetwork()->runPostInitFunctions();
```

And `SingleSampleBlock::process()` (NodeContainerTypes.h:993-1029):
```cpp
if (isBypassed())
{
    obj.getObject().process(data.as<FixProcessType>());  // block processing
}
else
{
    // frame processing with leftover handling
    obj.process(copy);
}
```

When bypassed, children are re-prepared with the original block size and process
the full block. This is the same A/B pattern as fixN_block.

`getBlockSizeForChildNodes()` (NodeContainerTypes.h:1057-1060):
```cpp
return isBypassed() ? originalBlockSize : 1;
```

### cpu-overhead-magnitude: Nature of per-sample overhead

The overhead is primarily from per-sample function call overhead. For each
sample in the block:
1. `FrameProcessor::next()` loads one float from each channel pointer into the
   frame span (for 1 channel: one load)
2. Call through `processFrame()` on the DynamicSerialProcessor, which iterates
   all child nodes calling `processFrame()` on each
3. `FrameProcessor::next()` writes the frame span back to the channel pointer
   (one store)

The interleave/deinterleave cost is minimal for 1 channel (just a load/store
per sample). The dominant cost is the per-sample function call chain through
the interpreted node list.

Compared to fix8_block: frame1_block calls processFrame() N times per block
(e.g., 512 times for a 512-sample buffer), while fix8_block calls process()
N/8 times (64 times). The ratio is 8:1 in call overhead. However, each
processFrame() call does less work (1 sample vs 8 samples), so the actual
overhead depends on how much per-call setup exists in child nodes.

The compiler can often inline processFrame() in compiled code, eliminating most
of the call overhead. The interpreted path has more overhead due to virtual
dispatch through the node list.

## Parameters

None.

## Conditional Behaviour

1. **Bypass:** Reverts to block processing with original block size (full re-prepare).
2. **Channel mismatch:** For frame1_block specifically, extra channels beyond
   channel 0 pass through unmodified. The fix<1> wrapper only processes channel 0.

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: []

Per-sample processing has significant overhead compared to block processing,
especially in interpreted mode. The overhead is proportional to host buffer
size (more samples = more iterations). In compiled mode, inlining reduces
this substantially.

## Notes

- frame1_block is the mono variant. For stereo processing, use frame2_block.
  In a stereo network, frame1_block only processes channel 0 -- channel 1
  passes through unmodified. This is often NOT what users want.
- The existing phase3 doc recommends fix8_block as a compromise between
  per-sample accuracy and CPU cost. This is a valid recommendation.
- `HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS` (default 8, defined in Macros.h:81)
  limits the maximum channel count for frame containers. frame1_block is well
  within this limit.
