MidiProcessor::exportState() -> String

Thread safety: UNSAFE -- ValueTree serialization and base64 encoding involve heap allocations.
Exports the full processor state (all parameters and internal state) as a
base64-encoded string. Works on any MIDI processor type.
Pair with:
  restoreState -- restore from the exported base64 string
  exportScriptControls -- lighter alternative for script processors (UI values only)
Source:
  ScriptingApiObjects.cpp:4698  exportState()
    -> ProcessorHelpers::getBase64String(mp, false, false)
