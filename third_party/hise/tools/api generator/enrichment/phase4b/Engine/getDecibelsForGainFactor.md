Engine::getDecibelsForGainFactor(double gainFactor) -> Double

Thread safety: SAFE -- pure math (Decibels::gainToDecibels)
Converts linear gain to decibels. 1.0 -> 0 dB, 0.5 -> ~-6 dB, 0.0 -> -100 dB.
Source:
  ScriptingApi.h  inline -> Decibels::gainToDecibels<double>(gainFactor)
