MessageHolder::ignoreEvent(Integer shouldBeIgnored) -> undefined

Thread safety: SAFE
Sets or clears the ignored flag (bit 30 of the 32-bit timestamp field). When set,
the event remains in the buffer but is skipped during processing. Unlike
Message.ignoreEvent() which operates on the live event, this modifies only the
stored copy. The flag has practical effect only if the event is later re-injected.

Dispatch/mechanics:
  Writes bit 30 of HiseEvent's uint32 timestamp field.
  setTimestamp/getTimestamp preserve flag bits independently -- setting the
  ignored flag does not corrupt the timestamp.

Pair with:
  Message.ignoreEvent -- ignore the live event in the callback
  Synth.addMessageFromHolder -- re-inject the stored event

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::ignoreEvent()
    -> e.ignoreEvent(shouldBeIgnored)
    -> sets/clears bit 30 of timestamp field
