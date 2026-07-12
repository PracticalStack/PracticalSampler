MessageHolder::setFineDetune(Number cents) -> undefined

Thread safety: SAFE
Sets fine detune in cents. Stored as int8 (-128..127). Values outside this range
silently truncated by cast. Used with coarse detune (semitones) to compute the final
pitch factor for voice playback.

Pair with:
  getFineDetune -- read fine detune
  setCoarseDetune -- set coarse detune in semitones

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setFineDetune()
    -> e.setFineDetune((int8)cents)
