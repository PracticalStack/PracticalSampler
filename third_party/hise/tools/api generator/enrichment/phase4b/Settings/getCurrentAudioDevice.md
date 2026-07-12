Settings::getCurrentAudioDevice() -> String

Thread safety: UNSAFE -- queries audio device manager, constructs a String
Returns the name of the currently active audio device.
Returns an empty string if no audio device is selected.

Pair with:
  getAvailableDeviceNames -- list all available devices
  setAudioDevice -- activate a device by name

Source:
  ScriptingApi.cpp  Settings::getCurrentAudioDevice()
    -> driver->deviceManager->getCurrentAudioDevice()->getName()
