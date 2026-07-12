Settings::getAvailableOutputChannels() -> Array

Thread safety: UNSAFE -- queries audio device, constructs String array on heap
Returns an array of output channel pair names (strings) for the current audio device.
Channel pairs are stereo pairs, not individual channels. Returns an empty array if
no audio device is active.

Dispatch/mechanics:
  driver->deviceManager->getCurrentAudioDevice()
    -> HiseSettings::ConversionHelpers::getChannelPairs(currentDevice)

Pair with:
  getCurrentOutputChannel -- read the active output channel pair index
  setOutputChannel -- select a different output channel pair

Source:
  ScriptingApi.cpp  Settings::getAvailableOutputChannels()
    -> ConversionHelpers::getChannelPairs()
