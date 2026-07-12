ContainerChild::setValue(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets this component's value in the Values tree without triggering the control
callback. The undo manager is explicitly bypassed (always nullptr). To trigger
the control callback, call changed() afterward. For undoable value changes, use
setValueWithUndo().
Dispatch/mechanics:
  data->getValueTree(Values).setPropertyExcludingListener(&valueListener, id, newValue, nullptr)
    -> excludes the valueListener so callback does NOT fire
Pair with:
  changed -- call after setValue to trigger the control callback
  setValueWithUndo -- alternative that uses the undo manager
  getValue -- reads back the value
Anti-patterns:
  - Do NOT expect setValue() to trigger the control callback -- it writes
    silently. Always call changed() afterward if you need the callback to fire.
Source:
  ScriptingApiContent.cpp  ChildReference::setValue()
    -> setPropertyExcludingListener(&valueListener, id, newValue, nullptr)
    -> undo manager explicitly nullptr (bypassed)
