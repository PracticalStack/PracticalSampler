MessageHolder::setCoarseDetune(Number semiToneDetune) -> undefined

Thread safety: SAFE
Sets coarse detune in semitones. Stored as int8 (-128..127). Values outside this
range are silently truncated by cast. Independent of transpose -- coarse detune is a
pitch modifier applied during voice rendering; transpose preserves NoteOn/NoteOff
matching.

Pair with:
  getCoarseDetune -- read coarse detune
  setFineDetune -- set fine detune in cents
  setTransposeAmount -- pitch shift that preserves note pairing

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setCoarseDetune()
    -> e.setCoarseDetune((int8)semiToneDetune)
