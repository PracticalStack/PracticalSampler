ScriptPanel::setPanelValueWithUndo(var oldValue, var newValue, String actionName) -> undefined

Thread safety: UNSAFE -- creates UndoableAction, modifies undo manager
Sets the panel value with undo support. For simple numeric values, creates an
UndoableControlEvent. For complex values (arrays, objects), creates a
PanelComplexDataUndoEvent. Both call setValue() + changed() on perform and undo.
Anti-patterns:
  - The oldValue parameter must match the current value for undo to restore
    correctly -- caller must pass the correct current value
Pair with:
  setValue -- non-undoable value setting
  setValueWithUndo -- simpler undo for numeric values
Source:
  ScriptingApiContent.cpp  ScriptPanel::setPanelValueWithUndo()
    -> UndoableControlEvent or PanelComplexDataUndoEvent
    -> MainController::getControlUndoManager()
