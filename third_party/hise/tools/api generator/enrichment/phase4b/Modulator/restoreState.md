Modulator::restoreState(String base64State) -> undefined

Thread safety: UNSAFE -- parses XML from base64 and applies full processor
state with property changes.
Restores the complete modulator state from a base64-encoded string previously
obtained from exportState(). Reports a script error if the string is invalid.

Pair with:
  exportState -- obtain the base64 string to pass to restoreState

Source:
  ScriptingApiObjects.cpp  restoreState()
    -> ValueTreeHelpers::getValueTreeFromBase64String() [validates]
    -> ProcessorHelpers::restoreFromBase64String()
