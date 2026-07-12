Settings::getAvailableBufferSizes() -> Array

Thread safety: UNSAFE -- queries audio device manager, constructs Array with heap allocations
Returns an array of available buffer sizes (integers) for the current audio device.
Returns an empty array if no audio device is active.

Dispatch/mechanics:
  driver->deviceManager->getCurrentAudioDevice()
    -> HiseSettings::ConversionHelpers::getBufferSizesForDevice(currentDevice)
    -> converts Array<int> to Array<var>

Pair with:
  getCurrentBufferSize -- read the active buffer size
  setBufferSize -- apply a new buffer size

Source:
  ScriptingApi.cpp  Settings::getAvailableBufferSizes()
    -> AudioIODevice* from deviceManager -> ConversionHelpers::getBufferSizesForDevice()
