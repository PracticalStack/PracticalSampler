MessageHolder::isPolyAftertouch() -> Integer

Thread safety: SAFE
Returns true if this event has type Aftertouch (5). Functionally identical to
isMonophonicAfterTouch() -- both check the same type field.

Anti-patterns:
  - Do NOT use this to distinguish poly from mono aftertouch -- both methods
    return the same result. Check the note number field instead: mono aftertouch
    typically has note number 0, poly has a non-zero note number.

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::isPolyAftertouch()
    -> e.isAftertouch() -> type == Type::Aftertouch
