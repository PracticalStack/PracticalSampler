Message::getFineDetune() -> Integer

Thread safety: SAFE
Returns the fine detune amount in cents stored on the current event. Per-event
property stored as int8 (-128 to 127). Used alongside coarse detune and transpose
to calculate final pitch via getPitchFactorForEvent(). Default is 0.

Pair with:
  setFineDetune -- set the fine detune value
  getCoarseDetune -- complementary coarse pitch adjustment in semitones

Source:
  ScriptingApi.cpp  Message::getFineDetune()
    -> constMessageHolder->getFineDetune()
    -> returns int8 cents field from HiseEvent DWord 2
