Message::setNoteNumber(Number newNoteNumber) -> undefined

Thread safety: SAFE
Sets the MIDI note number of the current note-on or note-off event. Value clamped to
0-127 via jmin<uint8>. Only works on NoteOn/NoteOff -- error for other types. The event
type check runs unconditionally (outside ENABLE_SCRIPTING_SAFE_CHECKS guard).

Anti-patterns:
  - Values above 127 are silently clamped. Negative values wrap via uint8 cast (e.g.,
    -1 becomes 255, then clamped to 127). Always pass 0-127.

Pair with:
  getNoteNumber -- read the current note number
  setTransposeAmount -- alternative: transpose without changing the raw note number

Source:
  ScriptingApi.cpp  Message::setNoteNumber()
    -> checks messageHolder->isNoteOnOrOff() (unconditional)
    -> messageHolder->setNoteNumber(jmin<uint8>(127, (uint8)newNoteNumber))
