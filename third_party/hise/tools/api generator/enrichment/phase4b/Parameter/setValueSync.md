DEPRECATED: Use setValue() (default mode).

Parameter::setValueSync(Double newValue) -> undefined

Thread safety: UNSAFE -- modifies ValueTree property with undo manager; triggers synchronous listener chain.
Stores the value to the parameter's ValueTree with undo support. The ValueTree
change triggers an internal listener that calls setValueAsync(), so
dynamicParameter->call() is eventually invoked. Use for UI-driven value changes
and preset recall.

Dispatch/mechanics:
  data.setProperty(Value, newValue, parent->getUndoManager())
    -> valuePropertyUpdater listener fires synchronously
    -> updateFromValueTree() -> setValueAsync(newValue)
    -> dynamicParameter->call(newValue)

Pair with:
  setValueAsync -- when immediate DSP update without undo is needed
  getValue -- read back the current value

Source:
  NodeBase.cpp:1112  Parameter::setValueSync()
    -> data.setProperty(PropertyIds::Value, newValue, parent->getUndoManager())
