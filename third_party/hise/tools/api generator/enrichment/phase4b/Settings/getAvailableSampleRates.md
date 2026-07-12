Settings::getAvailableSampleRates() -> Array

Thread safety: UNSAFE -- queries audio device, constructs String array on heap
Returns an array of supported sample rates for the current audio device.
Returns an empty array if no audio device is active.

Anti-patterns:
  - [BUG] Returns an array of strings (e.g., "44100", "48000"), not numbers.
    This differs from getAvailableBufferSizes() which returns integers. Parse
    values with parseInt() if arithmetic is needed.

Pair with:
  getCurrentSampleRate -- read the active sample rate
  setSampleRate -- apply a new sample rate

Source:
  ScriptingApi.cpp  Settings::getAvailableSampleRates()
    -> ConversionHelpers::getSampleRates(currentDevice)
