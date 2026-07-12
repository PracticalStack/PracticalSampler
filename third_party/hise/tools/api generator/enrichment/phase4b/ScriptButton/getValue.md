ScriptButton::getValue() -> var

Thread safety: SAFE
Returns the current value of the button (0 = off, 1 = on). Uses a
SimpleReadWriteLock for thread-safe read access.

Anti-patterns:
  - The stored value must not be a String -- assertion fires in debug builds

Source:
  ScriptingApiContent.cpp  ScriptComponent::getValue()
    -> SimpleReadWriteLock::ScopedReadLock -> returns cached value
