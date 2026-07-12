# container.multi -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/Container_Multi.h:91` (compiled), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:849` (interpreted)
**Base class:** `container_base<ParameterClass, Processors...>` (compiled), `ParallelNode` (interpreted)
**Classification:** container

## Signal Path

Parallel processing with channel splitting. The total channel count equals the
sum of all children's channel counts. Each child receives a non-overlapping
slice of the parent's channels via pointer offset (no copying).

```
Input (ch0, ch1, ch2, ch3)  [4 channels total]
  |
  +-- Child[0] processes (ch0, ch1)  [2 channels]
  +-- Child[1] processes (ch2, ch3)  [2 channels]
  |
Output (ch0, ch1, ch2, ch3)  [all channels modified in-place]
```

No buffer copying or summing required. Each child modifies its channel slice
in-place. Zero overhead beyond the child processing itself.

## Gap Answers

### channel-splitting-mechanism: Confirm channel model

**Confirmed.** Compiled version (Container_Multi.h:97):
`constexpr static int NumChannels = Helpers::getSummedChannels<Processors...>();`
Total channels = sum of all children's NumChannels.

Block processing (multiprocessor::Block, lines 46-65): maintains a `channelIndex`
counter. For each child, reads its `T::NumChannels` at compile time, creates a
`ProcessData<NumChannelsThisTime>` pointing at `d.getRawDataPointers() + channelIndex`,
copies non-audio data (events), calls `obj.process(thisData)`, then advances
`channelIndex += NumChannelsThisTime`.

Frame processing (multiprocessor::Frame, lines 67-88): uses `reinterpret_cast` to
create a `span<float, NumChannelsThisTime>` at the correct offset in the parent frame.

Interpreted version (NodeContainerTypes.cpp:577-603): same pattern with runtime
channel counts. Uses `n->getCurrentChannelAmount()` per child. Guards against
exceeding available channels: `if (endChannel <= d.getNumChannels())` -- children
that would exceed available channels are silently skipped (line 591).

### child-channel-determination: How child channel counts are determined

**Confirmed.** In the interpreted MultiChannelNode, each child's channel count is
determined by `n->getCurrentChannelAmount()` (NodeContainerTypes.cpp:587). Children
CAN have different channel counts (asymmetric splits are possible). For example,
child 0 could be mono (1 ch) and child 1 stereo (2 ch), requiring 3 total channels.

However, `prepare()` (lines 509-543) divides channels equally:
`numPerChildren = jmax(1, numNodes > 0 ? numChannels / numNodes : 0)`.
Each child receives the same `ps.numChannels = numPerChildren` during prepare.
This means prepare distributes evenly, but process uses the actual
`getCurrentChannelAmount()` per child.

If `numNodes > numChannels`, an error is thrown:
`Error::throwError(Error::TooManyChildNodes, numChannels, numNodes)` (line 517).

### event-handling: Event handling in multi

**Confirmed.** The interpreted `MultiChannelNode::handleHiseEvent()` correctly
creates a copy per child (NodeContainerTypes.cpp:550-557):
```cpp
for (auto n : nodes)
{
    HiseEvent c(e);
    n->handleHiseEvent(c);
}
```
Each child receives an independent copy.

The compiled version also creates a copy:
`HiseEvent copy(e); call_tuple_iterator1(handleHiseEvent, copy);`
(Container_Multi.h:127-129).

## Parameters

None. Multi has no parameters of its own.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

Multi adds zero buffer overhead -- it only adjusts channel pointers.
All CPU cost comes from the child nodes.

## Notes

- The `channelRanges[NUM_MAX_CHANNELS]` array in the interpreted version caches
  the channel range for each child. It is updated in `channelLayoutChanged()`.
- The `channelLayoutChanged()` method body is empty in the .cpp (lines 492-498),
  but the `channelRanges` are set during `prepare()` (lines 530-541).
- `currentChannelData[NUM_MAX_CHANNELS]` is a temporary array used during
  process() for building the channel pointer array per child.
