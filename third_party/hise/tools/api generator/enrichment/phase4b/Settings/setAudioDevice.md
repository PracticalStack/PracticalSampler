Settings::setAudioDevice(String name) -> undefined

Thread safety: UNSAFE -- delegates to AudioProcessorDriver which reconfigures the audio device
Sets the active audio output device by name. The name must match one of the strings
returned by getAvailableDeviceNames(). Primarily useful in standalone builds.

Pair with:
  getAvailableDeviceNames -- list valid device names
  getCurrentAudioDevice -- read the active device

Source:
  ScriptingApi.cpp  Settings::setAudioDevice()
    -> driver->setAudioDevice(name)
