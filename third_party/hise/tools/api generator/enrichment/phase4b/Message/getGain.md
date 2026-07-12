Message::getGain() -> Integer

Thread safety: SAFE
Returns the per-event gain in decibels. Stored as int8, range -100 to 36 dB (clamped
by setGain). -100 = silence. Default is 0. Works on any event type -- unlike getVelocity()
which requires a note event. Sound generators apply via getGainFactor() (dB to linear).

Pair with:
  setGain -- set the per-event gain
  getVelocity -- alternative for note events only (0-127 integer range)

Source:
  ScriptingApi.cpp  Message::getGain()
    -> constMessageHolder->getGain()
    -> returns int8 gain field from HiseEvent DWord 2
