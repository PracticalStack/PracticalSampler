Message::setVelocity(Number newVelocity) -> undefined

Thread safety: SAFE
Sets the velocity of the current note-on event. Only works on NoteOn -- reports an error
for NoteOff and all other event types (stricter than getVelocity which works on both).
Value cast to uint8 with no clamping. Setting to 0 does not convert to note-off (HiseEvent
uses explicit type fields), but downstream MIDI may interpret velocity 0 as note-off.

Anti-patterns:
  - Only works on note-on, not note-off. Use setGain() instead for per-event volume
    adjustments that work on any event type.
  - No range validation. Values outside 0-127 are truncated by uint8 cast.

Pair with:
  getVelocity -- read the current velocity
  setGain -- alternative that works on any event type

Source:
  ScriptingApi.cpp  Message::setVelocity()
    -> checks messageHolder->isNoteOn() (NoteOn only, not NoteOff)
    -> messageHolder->setVelocity((uint8)newVelocity)
