MessageHolder::isNoteOn() -> Integer

Thread safety: SAFE
Returns true if this event has type NoteOn (1). NoteOn with velocity 0 is still
classified as NoteOn (not auto-converted to NoteOff). Unique to MessageHolder --
Message does not need this because the onNoteOn callback identifies the event type.

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::isNoteOn()
    -> e.isNoteOn() -> type == Type::NoteOn
