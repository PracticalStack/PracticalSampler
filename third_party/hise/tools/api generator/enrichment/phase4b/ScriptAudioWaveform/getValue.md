ScriptAudioWaveform::getValue() -> var

Thread safety: SAFE
Returns the current value of the component. Uses SimpleReadWriteLock for
thread-safe read access.

Anti-patterns:
  - Do NOT store a String as the component value -- assertion fires in debug builds

Pair with:
  setValue -- to modify the value
  getValueNormalized -- to read in 0..1 range

Source:
  ScriptingApiContent.cpp  ScriptComponent::getValue()
    -> SimpleReadWriteLock::ScopedReadLock -> returns value
