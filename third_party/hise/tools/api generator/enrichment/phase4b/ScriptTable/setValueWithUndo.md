ScriptTable::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets the component value through the undo manager.

Dispatch/mechanics:
  Creates UndoableControlEvent and performs it through the configured UndoManager.

Pair with:
  setValue -- direct value write without undo event

Anti-patterns:
  - Do NOT call from control callbacks -- intended for explicit user actions that should become undo steps.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent::setValueWithUndo() -> UndoableControlEvent
