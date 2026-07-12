MessageHolder::getFineDetune() -> Integer

Thread safety: SAFE
Returns the fine detune in cents. Stored as int8 (-128..127). Used with coarse
detune (semitones) to compute the final pitch factor for voice playback.

Pair with:
  setFineDetune -- set fine detune in cents
  getCoarseDetune -- read coarse detune in semitones

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getFineDetune()
    -> (int)e.getFineDetune()
