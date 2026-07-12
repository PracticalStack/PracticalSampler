Message::setGain(Number gainInDecibels) -> undefined

Thread safety: SAFE
Sets the per-event gain in decibels. Clamped to -100..36 dB via jlimit. -100 = silence.
Works on any event type (unlike setVelocity which is note-on only), making it the preferred
per-event volume adjustment when velocity modification is too restrictive.

Anti-patterns:
  - Values outside -100..36 are silently clamped, no error. -100 dB produces effective
    silence (gain factor ~0.00001), not true zero.

Pair with:
  getGain -- read the current gain
  setVelocity -- alternative for note-on events only (integer 0-127)

Source:
  ScriptingApi.cpp  Message::setGain()
    -> messageHolder->setGain((int)jlimit<int>(-100, 36, gainInDecibels))
