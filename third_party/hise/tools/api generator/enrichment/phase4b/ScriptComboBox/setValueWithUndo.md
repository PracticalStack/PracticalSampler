ScriptComboBox::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets the value through the undo manager, creating an UndoableControlEvent.
Pass a 1-based integer index to select a combo box item.
Pair with:
  setValue -- non-undoable value setting
Anti-patterns:
  - Do NOT call from onControl callbacks -- intended for user-initiated value
    changes that should be undoable.
  - Do NOT pass a String value -- must be numeric.
Source:
  ScriptingApiContent.h  ScriptComponent::setValueWithUndo()
    -> creates UndoableControlEvent with old and new values
