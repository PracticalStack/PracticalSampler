Settings::getCurrentAudioDeviceType() -> String

Thread safety: UNSAFE -- queries audio device manager, constructs a String
Returns the type name of the current audio device (e.g., "ASIO", "Windows Audio",
"CoreAudio"). Returns an empty string if no audio device is selected.

Pair with:
  getAvailableDeviceTypes -- list all available driver types
  setAudioDeviceType -- switch to a different driver type

Source:
  ScriptingApi.cpp  Settings::getCurrentAudioDeviceType()
    -> driver->deviceManager->getCurrentAudioDeviceType()
