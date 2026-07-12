Message::getCoarseDetune() -> Integer

Thread safety: SAFE
Returns the coarse detune amount in semitones stored on the current event. Per-event
property stored as int8 (-128 to 127). Sound generators use this alongside fine detune
and transpose to calculate final pitch via getPitchFactorForEvent(). Default is 0.

Pair with:
  setCoarseDetune -- set the coarse detune value
  getFineDetune -- complementary fine pitch adjustment in cents
  getTransposeAmount -- separate transpose field (copied to note-off automatically)

Source:
  ScriptingApi.cpp  Message::getCoarseDetune()
    -> constMessageHolder->getCoarseDetune()
    -> returns int8 semitones field from HiseEvent DWord 2
