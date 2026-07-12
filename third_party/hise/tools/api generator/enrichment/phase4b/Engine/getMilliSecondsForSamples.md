Engine::getMilliSecondsForSamples(double samples) -> Double

Thread safety: SAFE -- inline arithmetic: samples / sampleRate * 1000.0
Converts a sample count to milliseconds at the current sample rate.
Pair with:
  getSamplesForMilliSeconds -- inverse
Source:
  ScriptingApi.h  inline -> samples / getSampleRate() * 1000.0
