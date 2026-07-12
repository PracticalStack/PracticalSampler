Engine::getSamplesForMilliSeconds(Number milliSeconds) -> Double

Thread safety: SAFE -- inline arithmetic
Converts milliseconds to samples: (ms / 1000.0) * getSampleRate().
Pair with:
  getMilliSecondsForSamples -- inverse
Source:
  ScriptingApi.h  inline -> (milliSeconds / 1000.0) * getSampleRate()
