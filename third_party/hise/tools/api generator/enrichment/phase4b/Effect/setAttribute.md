Effect::setAttribute(Number parameterIndex, Number newValue) -> undefined

Thread safety: WARNING -- uses context-dependent notification via getAttributeNotificationType(). On the audio thread, sets with dontSendNotification. On other threads, involves ValueTree property update with string lookup and notification dispatch.
Sets the parameter value at the given index. Use named constants for readable
access (e.g., fx.setAttribute(fx.Frequency, 1000.0)).
Required setup:
  const var fx = Synth.getEffect("MyEffect");
Dispatch/mechanics:
  effect->setAttribute(index, value, ProcessorHelpers::getAttributeNotificationType())
    -> notification type adapts to calling thread context automatically
Pair with:
  getAttribute -- read the current value
  getAttributeId / getAttributeIndex -- convert between name and index
Anti-patterns:
  - Do NOT use raw integer indices (fx.setAttribute(0, 1000.0)) -- use named
    constants (fx.setAttribute(fx.Frequency, 1000.0)) for refactor safety.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::setAttribute()
    -> effect->setAttribute(parameterIndex, newValue, getAttributeNotificationType())
