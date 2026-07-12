MessageHolder::setTransposeAmount(Number transposeValue) -> undefined

Thread safety: SAFE
Sets transpose in semitones. Cast to int8 (-128..127). Preserves the original note
number so NoteOn/NoteOff pairs match automatically. The sounding note is
getNoteNumber() + getTransposeAmount(). This is the recommended way to shift pitch.

Pair with:
  getTransposeAmount -- read the transpose offset
  setNoteNumber -- alternative: direct note number change (breaks note pairing)
  setCoarseDetune -- pitch modifier applied during rendering (no note pairing effect)

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setTransposeAmount()
    -> e.setTransposeAmount((int8)transposeValue)
