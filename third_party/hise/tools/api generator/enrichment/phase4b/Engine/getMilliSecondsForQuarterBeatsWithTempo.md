Engine::getMilliSecondsForQuarterBeatsWithTempo(double quarterBeats, double bpm) -> Double

Thread safety: SAFE -- pure math chain
Converts quarter beats to milliseconds at an explicit BPM.
Pair with:
  getMilliSecondsForQuarterBeats -- uses current host BPM
Source:
  ScriptingApi.cpp  Engine::getMilliSecondsForQuarterBeatsWithTempo()
    -> getSamplesForQuarterBeatsWithTempo() -> getMilliSecondsForSamples()
