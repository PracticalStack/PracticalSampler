MessageHolder::setVelocity(Number newVelocity) -> undefined

Thread safety: SAFE
Sets velocity. Cast to uint8 (0-255; standard MIDI is 0-127). No event-type guard --
on non-note events, overwrites the value byte (controller value for CC, pressure for
aftertouch). Setting velocity to 0 on a NoteOn does NOT auto-convert to NoteOff.

Pair with:
  getVelocity -- read the velocity
  setNoteNumber -- typically set together for NoteOn construction
  setGain -- per-event gain as alternative level control

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setVelocity()
    -> e.setVelocity((uint8)newVelocity)
