ScriptButton::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets the button's value (0 = off, 1 = on). Thread-safe -- can be called from any
thread; UI update happens asynchronously. Propagates value to all linked component
targets and sends value listener messages.

Anti-patterns:
  - Do NOT pass a String value -- reports a script error
  - If called during onInit, the value will NOT be restored after recompilation
    (skipRestoring is set to true)

Pair with:
  getValue -- read back the current value
  changed -- manually trigger the control callback after setValue
  setValueWithUndo -- set value with undo manager support

Source:
  ScriptingApiContent.cpp  ScriptComponent::setValue()
    -> SimpleReadWriteLock::ScopedWriteLock
    -> propagates to linked targets
    -> triggerAsyncUpdate() for UI
