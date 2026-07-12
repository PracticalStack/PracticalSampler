ScriptImage::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets the value through the undo manager, creating an UndoableControlEvent.
Pair with:
  setValue -- for non-undoable value changes
Anti-patterns:
  - Do NOT call from onControl callbacks -- intended for user-initiated changes only
  - Undo integration depends on useUndoManager property being enabled
Source:
  ScriptingApiContent.cpp  ScriptComponent::setValueWithUndo()
    -> creates UndoableControlEvent -> performs via UndoManager
