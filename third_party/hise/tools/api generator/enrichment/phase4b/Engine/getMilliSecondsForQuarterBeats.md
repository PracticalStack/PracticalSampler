Engine::getMilliSecondsForQuarterBeats(double quarterBeats) -> Double

Thread safety: SAFE -- pure math chain using current BPM (atomic read)
Converts quarter beats to milliseconds at the current host BPM and sample rate.
At 120 BPM, 1.0 quarter beat = 500 ms.
Pair with:
  getMilliSecondsForQuarterBeatsWithTempo -- explicit BPM variant
  getQuarterBeatsForMilliSeconds -- inverse
Source:
  ScriptingApi.cpp  Engine::getMilliSecondsForQuarterBeats()
    -> getSamplesForQuarterBeats() -> getMilliSecondsForSamples()
