MidiProcessor::restoreState(String base64State) -> undefined

Thread safety: UNSAFE -- base64 decoding, ValueTree parsing, and state restoration involve heap allocations.
Restores full processor state from a base64-encoded string previously generated
by exportState(). Validates the base64 by parsing into a ValueTree before
restoration. Works on any MIDI processor type.
Pair with:
  exportState -- generate the base64 string
  restoreScriptControls -- lighter alternative for script processors (UI values only, no recompile)
Source:
  ScriptingApiObjects.cpp:4706  restoreState()
    -> ValueTree parse validation
    -> ProcessorHelpers::restoreFromBase64String(mp, base64State, false)
