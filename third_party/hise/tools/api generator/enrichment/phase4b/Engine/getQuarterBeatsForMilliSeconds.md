Engine::getQuarterBeatsForMilliSeconds(double milliSeconds) -> Double

Thread safety: SAFE -- pure arithmetic chain, atomic BPM read
Converts milliseconds to quarter beats at the current host BPM.
At 120 BPM, 500 ms = 1.0 quarter beat.
Pair with:
  getMilliSecondsForQuarterBeats -- inverse
  getQuarterBeatsForMilliSecondsWithTempo -- explicit BPM variant
Source:
  ScriptingApi.cpp  -> getSamplesForMilliSeconds() -> getQuarterBeatsForSamples()
