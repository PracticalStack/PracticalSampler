Engine::getQuarterBeatsForMilliSecondsWithTempo(double milliSeconds, double bpm) -> Double

Thread safety: SAFE -- pure arithmetic chain
Converts milliseconds to quarter beats at an explicit BPM.
Pair with:
  getQuarterBeatsForMilliSeconds -- uses current host BPM
Source:
  ScriptingApi.cpp  -> getSamplesForMilliSeconds() -> getQuarterBeatsForSamplesWithTempo()
