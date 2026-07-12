MessageHolder::isMonophonicAfterTouch() -> Integer

Thread safety: SAFE
Returns true if this event has type Aftertouch (5). Delegates to
HiseEvent::isChannelPressure().

Anti-patterns:
  - Do NOT rely on this to distinguish mono from poly aftertouch --
    isMonophonicAfterTouch() and isPolyAftertouch() are functionally identical
    (both check type == Aftertouch). To distinguish, check the note number:
    mono aftertouch typically has note number 0, poly has a non-zero note number.

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::isMonophonicAfterTouch()
    -> e.isChannelPressure() -> type == Type::Aftertouch
