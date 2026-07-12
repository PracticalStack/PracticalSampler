# container.split -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/Container_Split.h:138` (compiled), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:782` (interpreted)
**Base class:** `container_base<ParameterClass, Processors...>` (compiled), `ParallelNode` (interpreted)
**Classification:** container

## Signal Path

Parallel processing with output summing. Each child receives a copy of the original
input and processes independently. Outputs are summed together.

```
Input --copy--> originalBuffer
  |
  +-- Child[0] processes data in-place (no copy needed for first child)
  |
  +-- Child[1..N] each: copy originalBuffer to workBuffer, process workBuffer, ADD result to data
  |
Output = Child[0] output + Child[1] output + ... + Child[N] output
```

First-child optimization: child 0 processes the original data buffer directly
(no copy needed since originalBuffer already holds a backup). Subsequent children
work on copies of the original and their outputs are added via
`FloatVectorOperations::add()`.

Single-child optimization: when only one child exists (`N == 1`), no buffer
copies are made at all -- the child processes the data in-place, identical to chain.

## Gap Answers

### parallel-dispatch-and-summing: Confirm split processing model

**Confirmed.** Compiled version (Container_Split.h:56-101):
1. `Block` constructor copies input to `originalBuffer` (line 70-73) unless single element
2. First child (channelCounter == 0): processes original data in-place (line 77-78)
3. Subsequent children: copy originalBuffer to workBuffer, process workBuffer,
   add result to main buffer via `FloatVectorOperations::add()` (lines 79-93)

Interpreted version (NodeContainerTypes.cpp:131-183):
Same pattern. Copies input to `original` heap buffer, first non-bypassed child
processes data in-place, subsequent non-bypassed children process copies of
original into workBuffer, results added via `FloatVectorOperations::add()`.

Single-child optimization confirmed at line 69 (`isSingleElement`) and line 77
(`channelCounter++ == 0`).

### bypass-behavior: SplitNode bypass handling

**Confirmed.** The interpreted `SplitNode::process()` checks `isBypassed()` at
entry and returns immediately (NodeContainerTypes.cpp:133). When the entire split
is bypassed, audio passes through unmodified.

Individual child bypass: the interpreted version checks `n->isBypassed()` in the
processing loop and skips bypassed children with `continue` (line 161-162). The
channelCounter still only increments for non-bypassed children, so the first
non-bypassed child always gets the in-place optimization.

The compiled version does not check individual child bypass (wrappers handle it).

### event-handling-interpreted: SplitNode event handling bug

**Confirmed bug.** The interpreted `SplitNode::handleHiseEvent()` creates a
`HiseEvent copy(e)` but passes `e` (the original) to `n->handleHiseEvent(e)`
(NodeContainerTypes.cpp:119-129). The copy is unused. This means the first child
CAN modify the event for subsequent children, making event handling behave like
chain (serial), not like a true parallel container.

Compare with `MultiChannelNode::handleHiseEvent()` which correctly creates
`HiseEvent c(e)` and passes `c` to each child (lines 550-557).

The compiled split passes a copy reference: `HiseEvent copy(e)` then iterates
with `call_tuple_iterator1(handleHiseEvent, copy)` (Container_Split.h:188-189).
This passes the same copy to all children, which is correct for the compiled path
since the tuple iterator evaluates left-to-right on the same reference.

This is already logged as Issue 7 in issues.md.

### buffer-allocation: Buffer allocation timing and memory cost

**Confirmed.** Both `originalBuffer` and `workBuffer` are `heap<float>` members
allocated during `prepare()`, not per-block.

Compiled (Container_Split.h:160-164): calls `FrameConverters::increaseBuffer()`
for both buffers when N > 1.

Interpreted (NodeContainerTypes.cpp:112-116): calls `DspHelpers::increaseBuffer()`
for both buffers when `blockSize > 0`.

Memory cost: each buffer = numChannels * blockSize * sizeof(float).
For stereo 512-sample: 2 * 512 * 4 = 4096 bytes per buffer, 8192 bytes total.
This is constant regardless of N (number of children) because the workBuffer is
reused for each subsequent child.

## Parameters

None. Split has no parameters of its own.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: [{ "parameter": "child count", "impact": "linear", "note": "each additional child adds one full process pass plus a buffer copy and add" }]

Overhead beyond child processing: one buffer copy on entry (originalBuffer),
one buffer copy + one add per additional child beyond the first.

## Notes

- Frame processing uses stack-allocated span copies instead of heap buffers.
- The interpreted SplitNode does not check `isBypassed()` on children in the
  frame processing path (`processFrameInternal`), only in the block path.
