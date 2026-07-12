Message::setFineDetune(Number cents) -> undefined

Thread safety: SAFE
Sets the fine detune in cents on the current event. Stored as int8 (-128 to 127). Sound
generators use this alongside coarse detune and transpose to calculate final pitch. Unlike
setTransposeAmount(), fine detune is NOT automatically copied from note-on to note-off
by EventIdHandler.

Pair with:
  getFineDetune -- read the current value
  setCoarseDetune -- complementary coarse pitch adjustment

Source:
  ScriptingApi.cpp  Message::setFineDetune()
    -> messageHolder->setFineDetune((int8)cents)
