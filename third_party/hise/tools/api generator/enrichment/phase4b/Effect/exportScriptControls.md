Effect::exportScriptControls() -> String

Thread safety: UNSAFE -- Base64 encoding involves heap allocation and string construction.
Serializes script UI control values as a Base64 string. Only works on Script FX
modules -- throws a script error on built-in (non-scripted) effects.
Pair with:
  restoreScriptControls -- restore the script control values
  exportState -- alternative that captures full processor state (works on all effects)
Anti-patterns:
  - Do NOT call on a built-in effect -- throws "exportScriptControls can only
    be used on Script Processors". Use exportState() instead.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::exportScriptControls()
    -> dynamic_cast<ProcessorWithScriptingContent*> check
    -> getBase64String(effect, true) (scriptOnly = true)
