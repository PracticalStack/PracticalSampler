Effect::isSuspended() -> Integer

Thread safety: SAFE
Returns whether the effect is currently suspended due to silence detection.
Returns true only when BOTH: the effect has opted into silence suspension
internally, AND it is currently suspended (after ~86 silent audio callbacks).
Anti-patterns:
  - Do NOT rely on this to detect silence -- always returns false for effects
    that have not opted into silence suspension via their internal
    isSuspendedOnSilence() flag. This opt-in is per effect type, not
    configurable from script.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::isSuspended()
    -> fx->isSuspendedOnSilence() && fx->isCurrentlySuspended()
