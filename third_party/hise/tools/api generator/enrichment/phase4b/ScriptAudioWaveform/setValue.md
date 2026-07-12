ScriptAudioWaveform::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets the component's value. Thread-safe -- can be called from any thread.
Propagates to linked component targets and triggers async UI update.

Anti-patterns:
  - Do NOT pass a String value -- reports a script error
  - If called during onInit, the value will NOT be restored after recompilation

Pair with:
  getValue -- to read the current value
  setValueNormalized -- to set via 0..1 range
  setValueWithUndo -- for undoable value changes

Source:
  ScriptingApiContent.cpp  ScriptComponent::setValue()
    -> SimpleReadWriteLock::ScopedWriteLock -> triggerAsyncUpdate()
    -> propagates to linked targets
