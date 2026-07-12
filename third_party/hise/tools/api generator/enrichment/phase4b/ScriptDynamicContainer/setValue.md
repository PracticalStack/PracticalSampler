ScriptDynamicContainer::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets the container's own ScriptComponent value. Thread-safe -- can be called from
any thread. This sets the container's own value, not dyncomp child values. Use
ContainerChild's setValue() for individual child component values.
Anti-patterns:
  - Do NOT pass a String value -- reports a script error.
  - If called during onInit, the value will NOT be restored after recompilation.
Pair with:
  getValue -- read back the container's value
  setValueWithUndo -- undo-capable version
  setValueCallback -- separate system for dyncomp child value changes
Source:
  ScriptingApiContent.cpp  ScriptComponent::setValue()
