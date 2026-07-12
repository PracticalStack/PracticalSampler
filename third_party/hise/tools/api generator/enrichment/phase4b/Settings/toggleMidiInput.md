Settings::toggleMidiInput(String midiInputName, Integer enableInput) -> undefined

Thread safety: UNSAFE -- delegates to AudioProcessorDriver which modifies MIDI device configuration
Enables or disables a MIDI input device by name. The name must match one of the
strings returned by getMidiInputDevices(). Primarily useful in standalone builds.

Pair with:
  getMidiInputDevices -- list available MIDI inputs
  isMidiInputEnabled -- check if a device is enabled

Source:
  ScriptingApi.cpp  Settings::toggleMidiInput()
    -> driver->toggleMidiInput(midiInputName, enableInput)
