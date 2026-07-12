Modulator::exportState() -> String

Thread safety: UNSAFE -- serializes the full processor state to XML and encodes
as base64, involving heap allocations.
Serializes the complete modulator state (all attributes, bypass state, child
processors) to a base64-encoded string.

Pair with:
  restoreState -- restore from the base64 string returned by exportState

Source:
  ScriptingApiObjects.cpp  exportState()
    -> ProcessorHelpers::getBase64String(mod, false)
