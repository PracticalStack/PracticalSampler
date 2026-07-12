Effect::restoreScriptControls(String base64Controls) -> undefined

Thread safety: UNSAFE -- state deserialization involves heap allocation and property restoration.
Restores script UI control values from a Base64 string previously obtained via
exportScriptControls(). Only works on Script FX modules -- throws a script
error on built-in (non-scripted) effects.
Pair with:
  exportScriptControls -- obtain the Base64 string to restore from
  restoreState -- alternative that restores full processor state (works on all effects)
Anti-patterns:
  - Do NOT call on a built-in effect -- throws "restoreScriptControls can only
    be used on Script Processors". Use restoreState() instead.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::restoreScriptControls()
    -> dynamic_cast<ProcessorWithScriptingContent*> check
    -> restoreFromBase64String with scriptOnly flag
