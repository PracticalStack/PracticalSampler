# control.voice_bang - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1099`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

MIDI note-on event -> handleHiseEvent() -> sends stored `value` to output parameter.

voice_bang stores the Value parameter and sends it to the output whenever a note-on event is received. The Value is the payload, not just a trigger.

## Gap Answers

### trigger-mechanism: How does voice_bang receive MIDI events without IsProcessingHiseEvent?

voice_bang implements `handleHiseEvent(HiseEvent& e)` directly in its class body, overriding the `SN_EMPTY_HANDLE_EVENT` inherited from `no_processing`. While `IsProcessingHiseEvent` is not registered in cppProperties, the node's handleHiseEvent method is wired into the OpaqueNode at creation time via `prototypes::static_wrappers`. The runtime checks for the method's existence, not just the property flag. The node DOES receive and respond to MIDI events.

Additionally, `prepare()` checks `ps.voiceIndex == nullptr || !ps.voiceIndex->isEnabled()` and throws `Error::IllegalMonophony` if the node is not in a polyphonic context. This means voice_bang REQUIRES a polyphonic container.

### value-at-trigger: Does it send the current Value or a fixed 1.0?

It sends the current stored `value` member. The `handleHiseEvent()` method calls `this->getParameter().call(value)` where `value` is set by `setValue(double input)`. So the Value parameter is the payload -- whatever value is currently set when the note-on arrives is what gets sent.

### polyphonic-context: Does it work in polyphonic containers?

The `prepare()` method explicitly requires a polyphonic context (throws `IllegalMonophony` if `voiceIndex` is null or disabled). Despite `IsPolyphonic` being false in cppProperties, the node requires being placed inside a polyphonic container. The `value` member is a simple double (not PolyData), so all voices share the same stored value. However, each voice's note-on triggers an independent send.

## Parameters

- **Value**: The value to send on note-on. Range 0..1. Default 0. Stored and sent as-is when triggered.

## CPU Assessment

baseline: negligible
polyphonic: false (no per-voice state, but requires polyphonic context)
scalingFactors: []

## Notes

The node throws an error during prepare() if not in a polyphonic container. This is the only control node that enforces polyphonic context requirement. The `value` member is shared across all voices, meaning changing Value between note-ons affects what the next voice receives.
