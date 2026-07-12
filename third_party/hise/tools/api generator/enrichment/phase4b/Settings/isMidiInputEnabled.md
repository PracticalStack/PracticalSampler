Settings::isMidiInputEnabled(String midiInputName) -> Integer

Thread safety: UNSAFE -- queries the device manager for MIDI input state
Returns whether the specified MIDI input device is currently enabled.
Returns false if no device manager is available.

Pair with:
  getMidiInputDevices -- list available MIDI inputs
  toggleMidiInput -- enable/disable a device

Source:
  ScriptingApi.cpp  Settings::isMidiInputEnabled()
    -> driver->deviceManager->isMidiInputEnabled(midiInputName)
