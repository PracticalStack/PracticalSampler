ChildSynth::restoreState(String base64State) -> undefined

Thread safety: UNSAFE -- base64 decoding, ValueTree deserialization, and processor state restoration involve heap allocations and lock acquisition
Restores the processor state from a base64-encoded string previously obtained via
exportState(). Validates the base64 string before restoring. Reports script error
"Can't load module state" if the string is invalid.
Pair with:
  exportState -- obtain the base64 string to restore from
Source:
  ScriptingApiObjects.cpp  restoreState()
    -> ProcessorHelpers::restoreFromBase64String(synth.get(), base64State)
    -> validates ValueTree parse before applying
