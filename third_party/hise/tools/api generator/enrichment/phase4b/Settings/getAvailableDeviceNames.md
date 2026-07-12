Settings::getAvailableDeviceNames() -> Array

Thread safety: UNSAFE -- queries audio device type for device names, constructs String array on heap
Returns an array of audio device names (strings) for the current audio device type.
Returns an empty array if no device type is configured.

Anti-patterns:
  - Do NOT assume device names are stable across sessions -- devices can be
    connected/disconnected. Always re-query before presenting to the user.

Pair with:
  getCurrentAudioDevice -- read the active device name
  setAudioDevice -- activate a device by name
  getAvailableDeviceTypes -- change device type first to get names for a different driver

Source:
  ScriptingApi.cpp  Settings::getAvailableDeviceNames()
    -> queries current AudioIODeviceType for device names
