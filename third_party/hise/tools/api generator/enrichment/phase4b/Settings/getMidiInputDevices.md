Settings::getMidiInputDevices() -> Array

Thread safety: UNSAFE -- calls MidiInput::getDevices() which queries system MIDI devices
Returns an array of available MIDI input device names (strings) detected by the system.

Pair with:
  toggleMidiInput -- enable/disable a device by name
  isMidiInputEnabled -- check if a device is enabled

Source:
  ScriptingApi.cpp  Settings::getMidiInputDevices()
    -> MidiInput::getDevices()
