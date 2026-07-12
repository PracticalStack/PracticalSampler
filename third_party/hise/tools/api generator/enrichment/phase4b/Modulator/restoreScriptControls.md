Modulator::restoreScriptControls(String base64Controls) -> undefined

Thread safety: UNSAFE -- parses base64 and restores UI control values with
property changes.
Restores the UI control values of a script modulator from a base64-encoded
string previously obtained from exportScriptControls(). Restores only control
values without recompiling the script.

Pair with:
  exportScriptControls -- obtain the base64 string to pass here

Anti-patterns:
  - Do NOT call on non-script modulators (LFO, AHDSR, etc.) -- reports a script
    error. Only ProcessorWithScriptingContent types are supported.

Source:
  ScriptingApiObjects.cpp  restoreScriptControls()
    -> ProcessorHelpers::restoreFromBase64String(mod, base64Controls, true)
