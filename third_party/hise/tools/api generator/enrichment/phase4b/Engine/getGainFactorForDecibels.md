Engine::getGainFactorForDecibels(double decibels) -> Double

Thread safety: SAFE -- pure inline math
Converts decibels to linear gain factor. 0 dB -> 1.0, -6 dB -> ~0.5012.
Returns 0.0 for values at or below -100 dB.
Source:
  ScriptingApi.h  inline -> Decibels::decibelsToGain<double>(decibels)
