Effect::exportState() -> String

Thread safety: UNSAFE -- Base64 encoding involves heap allocation and string construction.
Serializes the entire processor state (all parameters, internal data, child
processors) as a Base64 string. Captures the full module state, not just
script controls.
Pair with:
  restoreState -- restore from the Base64 string
Anti-patterns:
  - Do NOT use exportScriptControls on built-in effects -- it throws a script
    error. Use exportState instead.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::exportState()
    -> ProcessorHelpers::getBase64String(effect, false)
