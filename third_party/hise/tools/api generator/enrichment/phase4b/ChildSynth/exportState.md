ChildSynth::exportState() -> String

Thread safety: UNSAFE -- base64 encoding and ValueTree serialization involve heap allocations
Exports the complete processor state as a base64-encoded string. Captures all parameters,
modulator chain configuration, and internal processor state.
Pair with:
  restoreState -- reload the saved configuration from the base64 string
Source:
  ScriptingApiObjects.cpp  exportState()
    -> ProcessorHelpers::getBase64String(synth.get())
