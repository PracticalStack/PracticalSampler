ScriptTable::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets the generic ScriptComponent value and triggers linked updates.

Dispatch/mechanics:
  Writes value through ScriptComponent state path, triggers async UI update, and notifies linked targets/listeners.

Pair with:
  getValue -- read back stored value
  setValueWithUndo -- undo-aware setter for explicit user actions

Anti-patterns:
  - Do NOT pass String values -- call reports a script error.
  - Do NOT rely on onInit-set values being restored after recompilation -- init-time writes are not restored.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent::setValue() state write and async update path
