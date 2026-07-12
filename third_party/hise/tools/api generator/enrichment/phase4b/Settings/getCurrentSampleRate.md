Settings::getCurrentSampleRate() -> Double

Thread safety: UNSAFE -- queries audio device for current sample rate
Returns the current audio sample rate in Hz.
Returns -1 if no audio device is active.

Pair with:
  getAvailableSampleRates -- list supported sample rates
  setSampleRate -- apply a new sample rate

Source:
  ScriptingApi.cpp  Settings::getCurrentSampleRate()
    -> driver->getCurrentSampleRate()
