ScriptImage::getValue() -> var

Thread safety: SAFE
Returns the current value of the component. Uses SimpleReadWriteLock for
thread-safe read access.
Anti-patterns:
  - The stored value must not be a String -- assertion fires in debug builds
Pair with:
  setValue -- sets the value
Source:
  ScriptingApiContent.cpp  ScriptComponent::getValue()
    -> SimpleReadWriteLock::ScopedReadLock -> returns value
