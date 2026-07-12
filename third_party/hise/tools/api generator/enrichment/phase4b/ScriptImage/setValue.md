ScriptImage::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets the component's value. Thread-safe -- can be called from any thread.
Propagates to linked component targets. Triggers async UI update.
Pair with:
  getValue -- reads the value back
  changed -- triggers control callback with current value
  setValueWithUndo -- for undoable value changes
Anti-patterns:
  - Do NOT pass a String value -- reports a script error
  - If called during onInit, value will NOT be restored after recompilation
Source:
  ScriptingApiContent.cpp  ScriptComponent::setValue()
    -> SimpleReadWriteLock::ScopedWriteLock -> stores value
    -> propagates to linkedTargets -> triggerAsyncUpdate()
