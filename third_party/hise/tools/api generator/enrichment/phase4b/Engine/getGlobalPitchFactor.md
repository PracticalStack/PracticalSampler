Engine::getGlobalPitchFactor() -> Double

Thread safety: SAFE -- reads double, computes log2(x) * 12.0
Returns current global pitch factor in semitones (-12 to 12).
Pair with:
  setGlobalPitchFactor -- set the pitch offset
Source:
  ScriptingApi.cpp  Engine::getGlobalPitchFactor()
    -> log2(globalPitchFactor) * 12.0
