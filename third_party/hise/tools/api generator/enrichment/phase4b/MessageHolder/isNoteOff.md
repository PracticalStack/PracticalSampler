MessageHolder::isNoteOff() -> Integer

Thread safety: SAFE
Returns true if this event has type NoteOff (2). Unique to MessageHolder -- Message
does not need this because the onNoteOff callback already identifies the event type.

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::isNoteOff()
    -> e.isNoteOff() -> type == Type::NoteOff
