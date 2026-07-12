Engine::setGlobalPitchFactor(double pitchFactorInSemitones) -> undefined

Thread safety: SAFE -- clamps input, writes single double member variable
Sets global pitch factor in semitones affecting all voices. Clamped to -12..12.
Pair with:
  getGlobalPitchFactor -- read back the current value
Source:
  ScriptingApi.cpp  Engine::setGlobalPitchFactor()
    -> jlimit(-12.0, 12.0, semitones) -> globalPitchFactor = pow(2, st/12)
