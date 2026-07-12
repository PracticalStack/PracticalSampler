MessageHolder::getTransposeAmount() -> Integer

Thread safety: SAFE
Returns the transpose amount in semitones. Stored as int8 (-128..127). Transpose
shifts pitch without changing the original note number, so NoteOn/NoteOff pairs
match automatically. Sounding note = getNoteNumber() + getTransposeAmount().

Pair with:
  setTransposeAmount -- set the transpose offset
  getNoteNumber -- read the original note number

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getTransposeAmount()
    -> (int)e.getTransposeAmount()
