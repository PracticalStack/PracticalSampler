Threads::toString(Integer thread) -> String

Thread safety: WARNING -- String return value involves atomic ref-count operations.
Returns a human-readable name for the given thread constant.

Dispatch/mechanics:
  Switch on LockHelpers::Type:
    MessageLock -> "Message Thread"
    ScriptLock -> "Scripting Thread"
    SampleLock -> "Sample Thread"
    AudioLock -> "Audio Thread"
    numLockTypes -> "Unknown Thread"
    unused -> "Free (unlocked)"

Pair with:
  getCurrentThread -- get the constant to pass to toString

Source:
  ScriptingApi.cpp  toString()
    -> switch on (LockId)thread, returns corresponding string literal
