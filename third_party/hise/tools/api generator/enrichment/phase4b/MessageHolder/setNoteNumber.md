MessageHolder::setNoteNumber(Number newNoteNumber) -> undefined

Thread safety: SAFE
Sets the MIDI note number. Clamped to 0-127 via jmin<uint8>(). No event-type guard
-- calling on a CC event overwrites the controller number byte. Debug builds assert
isNoteOnOrOff() but the assignment proceeds regardless.

Anti-patterns:
  - Do NOT change the note number directly for pitch shifting -- this breaks
    automatic NoteOn/NoteOff matching. Use setTransposeAmount() instead, which
    preserves the original note number for pairing.

Pair with:
  getNoteNumber -- read the note number
  setTransposeAmount -- pitch shift that preserves note pairing

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setNoteNumber()
    -> e.setNoteNumber(jmin<uint8>(127, (uint8)newNoteNumber))
