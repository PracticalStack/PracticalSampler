ContainerChild::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets this component's value in the Values tree using the global undo manager,
without triggering the control callback. Unlike setValue(), this always uses the
global undo manager regardless of the component's useUndoManager property. Call
changed() afterward to trigger the control callback.
Anti-patterns:
  - Always uses the global undo manager, not the component's configured undo
    manager. Undo works even if useUndoManager is false on the component.
Pair with:
  setValue -- non-undoable alternative
  changed -- call afterward to trigger the control callback
Source:
  ScriptingApiContent.cpp  ChildReference::setValueWithUndo()
    -> getScriptProcessor()->getMainController_()->getControlUndoManager()
    -> setPropertyExcludingListener(&valueListener, id, newValue, globalUm)
