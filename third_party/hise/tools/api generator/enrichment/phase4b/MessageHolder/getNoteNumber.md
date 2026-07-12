MessageHolder::getNoteNumber() -> Integer

Thread safety: SAFE
Returns the MIDI note number (0-127 for note events). No event-type guard --
calling on a CC event returns the controller number byte.

Pair with:
  setNoteNumber -- set the note number
  getTransposeAmount -- read transpose offset (sounding note = getNoteNumber + getTransposeAmount)

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getNoteNumber()
    -> (int)e.getNoteNumber()
