ScriptModulationMatrix::setConnectionCallback(Function updateFunction) -> undefined

Thread safety: UNSAFE -- constructs a WeakCallbackHolder.
Registers a callback that fires whenever a modulation connection is added or
removed. The callback fires synchronously when the connection tree changes.
Callback signature: f(String sourceId, String targetId, bool wasAdded)

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  Stores WeakCallbackHolder(3 args) -> connectionListener on matrixData ValueTree
    -> fires on child add/remove with source name, target ID, and add/remove flag

Pair with:
  connect -- triggers this callback when connections change
  clearAllConnections -- triggers this callback for each removed connection

Source:
  ScriptModulationMatrix.cpp  setConnectionCallback()
    -> WeakCallbackHolder with 3 args
    -> connectionListener watches container->matrixData for child add/remove
