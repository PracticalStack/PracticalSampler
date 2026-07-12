Message::setCoarseDetune(Number semiToneDetune) -> undefined

Thread safety: SAFE
Sets the coarse detune in semitones on the current event. Stored as int8 (-128 to 127).
Sound generators use this alongside fine detune and transpose to calculate final pitch.
Unlike setTransposeAmount(), coarse detune is NOT automatically copied from note-on to
note-off by EventIdHandler.

Pair with:
  getCoarseDetune -- read the current value
  setFineDetune -- complementary fine pitch adjustment
  setTransposeAmount -- typically paired with opposite sign for timbre shifting

Source:
  ScriptingApi.cpp  Message::setCoarseDetune()
    -> messageHolder->setCoarseDetune((int8)semiToneDetune)
