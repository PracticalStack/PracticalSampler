# container.sidechain -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:936` (interpreted)
**Base class:** `SerialNode`
**Classification:** container

## Signal Path

Doubles the channel count by adding empty sidechain channels. Children see
2x the channels: the first half are the original audio, the second half are
zeroed sidechain input channels that can be filled by routing nodes.

```
Input (ch0, ch1)  [2 channels, stereo]
  |
  wrap::sidechain creates:
    ch0 = original left
    ch1 = original right
    ch2 = zeroed (sidechain left)
    ch3 = zeroed (sidechain right)
  |
  Children process all 4 channels serially
  |
Output (ch0, ch1)  [only original channels returned to parent]
```

## Gap Answers

### channel-doubling-mechanism: Confirm wrap::sidechain processing model

**Confirmed.** The `wrap::sidechain` wrapper (processors.h, per wrap-templates.md
section 3.9):

1. `prepare()`: allocates sideChainBuffer (unless frame mode), calls child
   prepare with `ps.numChannels *= 2` (NodeContainerTypes.cpp:745-751)
2. `process()`: creates a new channel pointer array of size `numChannels * 2`.
   First half: original audio channel pointers. Second half: zeroed sidechain
   buffer (one block per channel). Creates new ProcessData with doubled channels
   and forwards to child.
3. After processing, the parent only sees the original channels (the doubled
   ProcessData is local to the wrapper).

The interpreted `SidechainNode::prepare()` (lines 745-751) calls `obj.prepare(ps)`
first (which allocates the sidechain buffer), then `prepareNodes(ps)` with
`ps.numChannels *= 2` (children see doubled channels).

`getNumChannelsToDisplay()` returns `lastSpecs.numChannels * 2` (NodeContainerTypes.h:951),
confirming the doubled channel count is visible in the UI.

### frame-processing-limitation: Frame processing not supported

**Confirmed.** The `wrap::sidechain` wrapper asserts false in `processFrame()`
(per wrap-templates.md section 3.9). The interpreted `SidechainNode::processFrame()`
(NodeContainerTypes.cpp:765-769) has the FrameDataPeakChecker but delegates to
`obj.processFrame(data)` which will hit the assertion.

Additionally, `prepare()` skips sidechain buffer allocation when blockSize == 1
(frame mode). Sidechain containers cannot be nested inside frame-based containers.

### sidechain-buffer-content: Buffer persistence between blocks

**Confirmed.** The sidechain buffer channels are zeroed each `process()` call by
the wrapper. The wrap::sidechain implementation creates a new zeroed buffer region
for each process block. Any signal routed into these channels by routing nodes
persists only for that block -- it does not carry over to the next process call.

### description-grammar: Description text issue

The base data description reads "Creates a empty audio by duplicating the channel
amount for sidechain routing." This has a grammar error ("a empty" should be
"an empty"). Additionally the phrasing could be clearer. Suggested replacement:
"Doubles the channel count by adding empty sidechain channels."

## Parameters

None. Sidechain has no parameters of its own.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

Overhead: one buffer zeroing operation per block for the sidechain channels,
plus the channel pointer array setup. Children process double the channels,
which may increase their CPU cost.

## Notes

- The typical workflow: (1) wrap effects in sidechain container, (2) add a
  dynamics node with Sidechain parameter enabled, (3) use routing nodes
  (routing.receive) to fill the extra channels with sidechain input.
- `getBlockSizeForChildNodes()` returns `originalBlockSize` unchanged
  (NodeContainerTypes.cpp:771-774) -- only channel count is modified.
