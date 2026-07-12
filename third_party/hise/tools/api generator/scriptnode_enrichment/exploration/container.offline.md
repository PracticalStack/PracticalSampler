# container.offline -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:176` (interpreted)
**Base class:** `SerialNode`
**Classification:** container

## Signal Path

No realtime audio processing. The `wrap::offline` wrapper makes `process()`,
`processFrame()`, and `handleHiseEvent()` all no-ops. Only `prepare()`, `reset()`,
and `initialise()` are forwarded to children.

```
Input audio --> [no processing, passthrough] --> Output audio
               Children receive prepare/reset but never process
```

The offline container is designed for nodes that render to external data objects
(AudioFiles, SliderPacks, etc.) outside of the realtime audio callback.
The actual rendering is triggered by a separate mechanism, not through the
standard `process()` callback.

## Gap Answers

### offline-processing-model: How children process audio

**Confirmed.** The interpreted `OfflineChainNode`:

- `process()` (NodeContainerTypes.cpp:839-843): Only creates NodeProfiler and
  ProcessDataPeakChecker -- no actual processing. Audio passes through unmodified.
- `processFrame()` (lines 834-837): Only creates FrameDataPeakChecker -- no-op.
- `handleHiseEvent()` (lines 851-853): Completely empty body.
- `prepare()` (lines 845-849): Calls `NodeBase::prepare(ps)` and
  `NodeContainer::prepareNodes(ps)` -- children ARE prepared normally.
- `reset()` (lines 855-858): Calls `obj.reset()` -- children are reset normally.

The `wrap::offline` wrapper (processors.h, per wrap-templates.md section 3.19)
confirms this pattern: process, processFrame, and handleHiseEvent are empty.
Only prepare, reset, and initialise are forwarded.

### offline-rendering-mechanism: How rendering is triggered

The offline container provides a prepared node tree that can be invoked
programmatically outside the realtime audio callback. The exact trigger
mechanism depends on the context -- typically an external API call or a
user-initiated action that feeds audio data through the prepared children.

The children inside the offline container have valid PrepareSpecs (sample rate,
block size, channels) from the normal prepare chain, so they can process audio
when explicitly invoked. The offline container simply prevents them from being
called during the normal realtime audio callback.

This pattern is useful for offline rendering tasks: sample analysis, file
processing, convolution IR generation, etc.

### missing-description: Description text

The base data has an empty description string. Based on C++ analysis, an
appropriate description would be: "A container for offline (non-realtime)
processing that skips the realtime audio callback."

## Parameters

None. Offline has no parameters of its own.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

Zero CPU cost during realtime processing -- all audio callbacks are no-ops.

## Notes

- The offline container has the least documentation of any container node:
  empty description in base data, STUB phase3 doc (6 lines).
- The `wrap::offline` wrapper uses `SN_OPAQUE_WRAPPER` (transparent).
- Children are fully prepared and can be invoked outside the realtime callback
  for offline rendering tasks.
- The constructor calls `obj.initialise(this)` (NodeContainerTypes.cpp:831),
  connecting the wrapper to the value tree system.
