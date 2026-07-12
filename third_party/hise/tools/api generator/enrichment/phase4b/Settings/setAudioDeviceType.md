Settings::setAudioDeviceType(String deviceName) -> undefined

Thread safety: UNSAFE -- reconfigures the audio driver type
Sets the audio device driver type by name (e.g., "ASIO", "Windows Audio", "CoreAudio").
Changing the device type resets the active audio device. Primarily useful in standalone builds.

Pair with:
  getAvailableDeviceTypes -- list valid driver types
  getCurrentAudioDeviceType -- read the active driver type

Source:
  ScriptingApi.cpp  Settings::setAudioDeviceType()
    -> driver->setAudioDeviceType(deviceName)
