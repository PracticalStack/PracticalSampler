Modulator::exportScriptControls() -> String

Thread safety: UNSAFE -- serializes UI control values to base64, involving
heap allocations.
Serializes the UI control values of a script modulator to a base64-encoded
string. Only works on script modulators (Script Voice Start Modulator, Script
Time Variant Modulator, Script Envelope Modulator).

Pair with:
  restoreScriptControls -- restore control values from the returned string

Anti-patterns:
  - Do NOT call on non-script modulators (LFO, AHDSR, etc.) -- reports a script
    error. Only ProcessorWithScriptingContent types are supported.

Source:
  ScriptingApiObjects.cpp  exportScriptControls()
    -> ProcessorHelpers::getBase64String(mod, false, true)
