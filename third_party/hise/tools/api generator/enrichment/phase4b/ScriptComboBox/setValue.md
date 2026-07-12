ScriptComboBox::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets the combo box value as a 1-based integer index. Thread-safe -- can be called
from any thread; UI update happens asynchronously. Propagates to linked component
targets.
Dispatch/mechanics:
  ScriptComponent::setValue(newValue)
    -> stores value, sets skipRestoring if called during onInit
    -> triggerAsyncUpdate() for UI refresh
    -> notifies value listeners and linked targets
Pair with:
  getValue -- read back the current selection index
  getItemText -- get the display text of the current selection
  changed -- trigger the control callback after programmatic setValue
Anti-patterns:
  - Do NOT pass a String value -- reports a script error.
  - Do NOT use setValue(0) to select the first item -- value 0 means "nothing selected".
    Use setValue(1) for the first item.
  - If called during onInit, the value will NOT be restored after recompilation
    (skipRestoring is set to true).
Source:
  ScriptingApiContent.h  ScriptComponent::setValue()
    -> no override in ScriptComboBox; uses base implementation directly
