Settings::setSampleRate(Double sampleRate) -> undefined

Thread safety: UNSAFE -- reconfigures audio device sample rate through the driver
Sets the audio sample rate in Hz. The value should be one of the rates returned
by getAvailableSampleRates(). Primarily useful in standalone builds.

Pair with:
  getAvailableSampleRates -- list supported sample rates
  getCurrentSampleRate -- read the active sample rate

Source:
  ScriptingApi.cpp  Settings::setSampleRate()
    -> driver->setCurrentSampleRate(sampleRate)
