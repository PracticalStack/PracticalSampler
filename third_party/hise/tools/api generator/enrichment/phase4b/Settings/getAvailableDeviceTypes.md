Settings::getAvailableDeviceTypes() -> Array

Thread safety: UNSAFE -- iterates device type array, constructs String array on heap
Returns an array of available audio device type names (strings), such as
"Windows Audio", "ASIO", "DirectSound", or "CoreAudio" depending on platform.

Pair with:
  getCurrentAudioDeviceType -- read the active device type
  setAudioDeviceType -- activate a different driver type

Source:
  ScriptingApi.cpp  Settings::getAvailableDeviceTypes()
    -> iterates deviceManager->getAvailableDeviceTypes()
