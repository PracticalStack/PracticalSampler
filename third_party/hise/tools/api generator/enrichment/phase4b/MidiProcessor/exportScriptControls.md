MidiProcessor::exportScriptControls() -> String

Thread safety: UNSAFE -- ValueTree serialization and base64 encoding involve heap allocations.
Exports only the UI control values (scripting content) of the MIDI processor as
a base64-encoded string. Only works on script processors (JavascriptMidiProcessor).
Unlike exportState(), restoring via restoreScriptControls() does not trigger recompilation.
Pair with:
  restoreScriptControls -- restore the exported UI control values
  exportState -- full state alternative that works on all module types
Anti-patterns:
  - Do NOT call on built-in MIDI modules (Transposer, Arpeggiator) -- throws "exportScriptControls can only be used on Script Processors". Use exportState() instead.
Source:
  ScriptingApiObjects.cpp:4724  exportScriptControls()
    -> dynamic_cast<ProcessorWithScriptingContent*> guard
    -> ProcessorHelpers::getBase64String(mp, false, true)  // true = exportContentOnly
