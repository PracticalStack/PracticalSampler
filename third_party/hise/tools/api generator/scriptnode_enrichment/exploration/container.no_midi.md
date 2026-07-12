# container.no_midi -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:125` (interpreted)
**Base class:** `SerialNode`
**Classification:** container

## Signal Path

Serial processing identical to chain. The ONLY difference is that
`handleHiseEvent()` is a no-op -- MIDI events are blocked from reaching children.

```
Input audio --> Child[0].process -> Child[1].process -> ... -> Output
MIDI events --> [blocked by wrap::no_midi] --> children receive nothing
```

## Gap Answers

### midi-blocking-mechanism: Confirm wrap::no_midi behavior

**Confirmed.** `NoMidiChainNode` wraps `DynamicSerialProcessor` in
`wrap::no_midi<>` (NodeContainerTypes.h:143). The `wrap::no_midi` template
(processors.h, per wrap-templates.md section 3.15) makes `handleHiseEvent()`
a no-op. All other callbacks (prepare, reset, process, processFrame) are
forwarded normally.

The interpreted `NoMidiChainNode::handleHiseEvent()` (NodeContainerTypes.cpp:1630-1634)
delegates to `obj.handleHiseEvent(e)` with the comment "let the wrapper send it
to nirvana" -- confirming the wrapper silently drops the event.

Audio processing is completely unaffected. Children process audio normally
through `DynamicSerialProcessor` (same serial iteration as chain).

### polyphonic-context-use-case: Event type blocking scope

**Confirmed.** The `wrap::no_midi` wrapper blocks ALL event types by making
`handleHiseEvent()` a complete no-op. This includes:
- Note-on / Note-off
- CC (control change)
- Pitch wheel
- Aftertouch (channel and polyphonic)
- Any other HiseEvent type

There is no selective filtering -- it is all-or-nothing. The typical use case
is preventing oscillators from receiving MIDI frequency changes when they are
used as LFOs in a polyphonic synthesiser context.

## Parameters

None. No_midi has no parameters of its own.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

No_midi adds zero overhead. The no-op handleHiseEvent is a trivial function.

## Notes

- No_midi is the inverse of midichain: where midichain enables MIDI processing,
  no_midi disables it.
- No_midi uses `SN_OPAQUE_WRAPPER` (transparent), meaning the wrapper is
  invisible to type queries.
