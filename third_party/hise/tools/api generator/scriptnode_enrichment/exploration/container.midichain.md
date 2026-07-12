# container.midichain -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:100` (interpreted)
**Base class:** `SerialNode`
**Classification:** container

## Signal Path

Serial processing identical to chain, plus sample-accurate MIDI event splitting.
The `wrap::event` wrapper splits the audio block at MIDI event timestamps so that
`handleHiseEvent()` is called at the correct sample position within the block.

```
Input audio block (512 samples, 3 MIDI events at samples 100, 200, 350):
  |
  process(samples 0-99)
  handleHiseEvent(event at 100)
  process(samples 100-199)
  handleHiseEvent(event at 200)
  process(samples 200-349)
  handleHiseEvent(event at 350)
  process(samples 350-511)
  |
Output
```

## Gap Answers

### event-splitting-mechanism: Confirm wrap::event processing model

**Confirmed.** The `wrap::event` template (processors.h, documented in
infrastructure/wrap-templates.md section 3.8) implements sample-accurate
MIDI splitting:

1. Gets event list via `data.toEventData()`
2. If no events: forwards directly to child's `process()`
3. If events exist: uses `ChunkableProcessData` to split at event timestamps.
   For each event (ordered by timestamp):
   a. Process audio from last position to event timestamp
   b. Call `handleHiseEvent()` on the event
   c. Continue to next event
4. Process remaining audio after last event

Multiple events at the same timestamp are handled sequentially (each triggers
a handleHiseEvent call, but the audio chunk between same-timestamp events has
zero length).

The interpreted `MidiChainNode::process()` delegates to `obj.process(data)`
when not bypassed (NodeContainerTypes.cpp:793-806). When bypassed, it calls
`obj.getObject().process(data)` which bypasses the event wrapper and processes
audio without MIDI splitting.

### serial-processing-within: Confirm midichain = chain + MIDI

**Confirmed.** MidiChainNode wraps `DynamicSerialProcessor` in `wrap::event`
(NodeContainerTypes.h:122). The `DynamicSerialProcessor` is the same class used
by ChainNode -- it iterates children sequentially with a for-each loop. The ONLY
difference from chain is the `wrap::event` wrapper that adds MIDI event splitting.

Audio processing (excluding the event splitting) is identical to chain.

### context-requirement: When midichain is necessary vs redundant

The `wrap::event` wrapper sets `isProcessingHiseEvent = true` (always, per
wrap-templates.md section 3.8). This enables MIDI event forwarding for the
entire subtree.

In synthesiser/envelope/time-variant-modulator contexts, MIDI processing is
typically already enabled by the parent context. Using midichain there is
redundant but harmless -- it adds an extra layer of event splitting that may
split at slightly different granularity but produces the same functional result.

In effect plugin contexts (`FRONTEND_IS_PLUGIN`), MIDI processing is disabled
by default. Midichain is the correct way to enable MIDI event handling for
nodes in an effect context.

The `prepare()` method (NodeContainerTypes.cpp:808-815) calls
`DspHelpers::setErrorIfFrameProcessing(ps)` and
`DspHelpers::setErrorIfNotOriginalSamplerate(ps, this)`, indicating midichain
should not be nested inside frame-based or resampled containers.

## Parameters

None. Midichain has no parameters of its own.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: [{ "parameter": "MIDI event density", "impact": "linear", "note": "more events = more audio chunk boundaries = more process() calls with smaller blocks" }]

The event splitting adds overhead proportional to the number of MIDI events
per block. With zero events, the cost is a single conditional check.

## Notes

- Midichain uses `Colour(MIDI_PROCESSOR_COLOUR)` for its container color.
- The `prepare()` method validates that midichain is not inside a frame-based
  container or a resampled context. Both would interfere with the
  timestamp-based audio splitting.
