MessageHolder::getGain() -> Integer

Thread safety: SAFE
Returns per-event gain in decibels. Stored as int8 (-128..127). A value of 0 means
unity gain. Applied as per-voice gain factor during rendering.

Pair with:
  setGain -- set the per-event gain

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getGain()
    -> (int)e.getGain()
