MidiProcessor::setAttribute(int parameterIndex, float value) -> undefined

Thread safety: UNSAFE -- triggers ValueTree property change with notifications, which may involve string lookups and listener dispatch.
Sets the value of the parameter at the given index. Use dynamic constants
(mp.Intensity) instead of raw indices. Bracket syntax (mp["Intensity"] = 0.5)
also delegates to this method.
Dispatch/mechanics:
  mp->setAttribute(index, value, ProcessorHelpers::getAttributeNotificationType())
    -> notification type determined automatically by calling context (async on audio thread, sync on message thread)
Pair with:
  getAttribute -- read the current parameter value
  getAttributeIndex -- convert string name to index
Anti-patterns:
  - Do NOT use raw index numbers (mp.setAttribute(0, v)) -- use dynamic constants (mp.setAttribute(mp.Intensity, v)) for readability and safety
  - Do NOT read via bracket syntax (var v = mp["Intensity"]) -- always returns 1.0 due to incomplete getAssignedValue(). Use getAttribute() instead.
Source:
  ScriptingApiObjects.cpp:4635  setAttribute()
    -> mp->setAttribute(index, value, ProcessorHelpers::getAttributeNotificationType())
