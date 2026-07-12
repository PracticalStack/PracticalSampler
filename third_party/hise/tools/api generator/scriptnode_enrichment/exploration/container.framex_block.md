# container.framex_block -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/processors.h:592` (wrap::frame_x), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:913` (SingleSampleBlockX)
**Base class:** `SerialNode` (interpreted)
**Classification:** container

## Signal Path

Serial container that converts block-based processing to per-sample (frame)
processing with a dynamic channel count. Unlike frame1_block/frame2_block which
fix the channel count at compile time, framex_block adapts to the current
network channel count at runtime.

```
Input block (N samples, C channels)
  -> frame_x: for each sample:
    -> children.processFrame(span<float, C>{ch0, ch1, ..., chC-1})
  -> Output
```

Uses `wrap::frame_x<T>` directly (processors.h:592), NOT the `wrap::frame<C, T>`
alias. No `fix<C>` wrapper -- channel count is runtime-determined.

## Gap Answers

### dynamic-channel-dispatch: How processFrame() handles dynamic channels

`wrap::frame_x::process()` (processors.h:617-625):
```cpp
template <typename ProcessDataType> void process(ProcessDataType& data)
{
    constexpr int C = ProcessDataType::getNumFixedChannels();
    if constexpr (ProcessDataType::hasCompileTimeSize())
        FrameConverters::processFix<C>(&obj, data);
    else
        FrameConverters::forwardToFrame16(&obj, data);
}
```

For the interpreted `SingleSampleBlockX`, the ProcessDataType is `ProcessDataDyn`
which does NOT have compile-time size. So it uses `FrameConverters::forwardToFrame16()`.

`forwardToFrame16()` (snex_ProcessDataTypes.h:633-655) is a runtime switch on
`data.getNumChannels()`:
```cpp
switch (data.getNumChannels())
{
case 1:  processFix<1>(ptr, data); break;
case 2:  processFix<2>(ptr, data); break;
case 3:  processFix<3>(ptr, data); break;
...
case 16: processFix<16>(ptr, data); break;
default: jassertfalse;
}
```

Each case instantiates a `processFix<N>()` which creates a `FrameProcessor<N>`
for interleaved iteration. The dispatch happens once per process() call (per
host buffer), not per sample.

### max-channel-count: Maximum supported channels

The maximum is controlled by `HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS`, defined
in `hi_tools/Macros.h:81` as **8** by default.

The `allowFrameContainerChannel<N>()` template (snex_ProcessDataTypes.h:603)
returns `N <= HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS`. The switch in
`forwardToFrame16()` has cases up to 16, but each case is guarded by
`if constexpr (allowFrameContainerChannel<N>())`. With the default limit of 8,
cases 9-16 compile to empty cases that fall through to the `jassertfalse`.

So the effective maximum is **8 channels** by default. Channels beyond 8
trigger a debug assertion.

### performance-vs-fixed-frame: Overhead vs frame1/frame2_block

The dynamic dispatch adds one switch branch per process() call (not per sample).
This is negligible compared to the per-sample processing overhead.

The more significant difference is that compiled code using `wrap::frame<2, T>`
can fully inline the channel count, allowing the compiler to optimise the
interleaving loop. With `wrap::frame_x<T>`, the channel count is runtime and
the compiler cannot eliminate the per-channel loop bounds check.

In practice, for 1-2 channels the performance difference is small. For higher
channel counts (4+), the fixed variants may have a measurable advantage due
to SIMD alignment opportunities.

### bypass-behaviour: Bypass mechanism

Identical to frame1_block. `SingleSampleBlockX::setBypassed()` (NodeContainerTypes.cpp:681-697):
```cpp
SerialNode::setBypassed(shouldBeBypassed);
PrepareSpecs ps;
ps.blockSize = originalBlockSize;
// ...
prepare(ps);
getRootNetwork()->runPostInitFunctions();
```

`SingleSampleBlockX::process()` (NodeContainerTypes.cpp:710-720):
```cpp
if (isBypassed())
    obj.getObject().process(data);  // block processing
else
    obj.process(data);              // frame processing
```

`getBlockSizeForChildNodes()` (NodeContainerTypes.cpp:728-731):
```cpp
return isBypassed() ? originalBlockSize : 1;
```

### description-accuracy: Description assessment

The base data description "Enables per sample processing for the child nodes."
is accurate but incomplete. It should mention that channel count adapts to the
network context, unlike frame1_block/frame2_block which fix it. This is the
key distinguishing feature.

## Parameters

None.

## Conditional Behaviour

1. **Bypass:** Reverts to block processing (same as frame1_block).
2. **Channel count:** Determined at runtime from the network. The switch in
   forwardToFrame16() dispatches to the correct fixed-channel processFix<N>().

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: []

Same per-sample overhead as frameN_block variants. Marginal additional cost
from runtime channel dispatch (one switch per process() call). Less compiler
optimisation opportunity than fixed-channel variants.

## Notes

- framex_block holds an `AudioSampleBuffer leftoverBuffer` member
  (NodeContainerTypes.h:933) but the interpreted `process()` and `prepare()`
  implementations (NodeContainerTypes.cpp:699-720) do not use it for channel
  mismatch handling. The leftover buffer appears to be declared but unused in
  the interpreted path for this node.
- `HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS` can be overridden at compile time to
  support more channels, but the default of 8 covers all standard use cases.
- This is the node that `container.dynamic_blocksize` with BlockSize=1 is
  functionally equivalent to -- both use `FrameConverters::forwardToFrame16()`
  for dynamic-channel frame processing.
