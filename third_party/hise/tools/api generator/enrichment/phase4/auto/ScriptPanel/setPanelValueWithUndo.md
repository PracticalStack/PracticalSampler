Sets the panel value with undo support, creating an undoable action. For simple numeric values this creates an `UndoableControlEvent`; for complex values (arrays, objects) it creates a `PanelComplexDataUndoEvent`. Both call `setValue()` and `changed()` on perform and undo.

> [!Warning:Pass correct current value for undo] The `oldValue` parameter must match the panel's current value for undo to restore correctly. It is the caller's responsibility to capture and pass the correct current value before the change.
