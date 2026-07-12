MessageHolder::getCoarseDetune() -> Integer

Thread safety: SAFE
Returns the coarse detune in semitones. Stored as int8 (-128..127). Used with fine
detune (cents) to compute the final pitch factor for voice playback.

Pair with:
  setCoarseDetune -- set the coarse detune
  getFineDetune -- read fine detune in cents

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getCoarseDetune()
    -> (int)e.getCoarseDetune()
