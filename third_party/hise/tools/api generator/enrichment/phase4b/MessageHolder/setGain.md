MessageHolder::setGain(Number gainInDecibels) -> undefined

Thread safety: SAFE
Sets per-event gain in decibels. Clamped to -100..36 dB by jlimit, then stored as
int8. 0 = unity gain. Converted to a linear gain factor during voice rendering.

Pair with:
  getGain -- read the gain value
  setVelocity -- alternative: velocity-based level control

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setGain()
    -> e.setGain(jlimit(-100, 36, (int)gainInDecibels))
