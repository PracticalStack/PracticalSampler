Message::isPolyAftertouch() -> Integer

Thread safety: SAFE
Returns whether the current event is a polyphonic aftertouch message. Used inside
onController before calling getPolyAfterTouchNoteNumber()/getPolyAfterTouchPressureValue().

Anti-patterns:
  - Returns true for both poly aftertouch AND monophonic channel pressure because both
    use the same Type::Aftertouch enum. isMonophonicAfterTouch() has identical behavior.
    Neither method can reliably distinguish the two aftertouch types at the type level.

Source:
  ScriptingApi.cpp  Message::isPolyAftertouch()
    -> constMessageHolder->isAftertouch()
    -> checks type == Type::Aftertouch (same as isChannelPressure)
