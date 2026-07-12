ScriptModulationMatrix::setDragCallback(Function newDragCallback) -> undefined

Thread safety: UNSAFE -- constructs a WeakCallbackHolder and registers a broadcaster listener.
Registers a callback that fires during drag-and-drop interaction with modulation
connections. Any previously registered drag callback is removed first.
Callback signature: f(String sourceId, String targetId, String action)

Action values:
  "DragStart"       A modulation source drag operation has begun
  "DragEnd"         The drag operation was cancelled or completed
  "Drop"            The source was dropped on a valid target
  "Hover"           The dragged source is hovering over a valid target
  "DisabledHover"   The dragged source is hovering over an invalid/occupied target

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  WeakCallbackHolder(3 args) -> connected to container->dragBroadcaster
    -> DragAction enum values mapped to action strings

Pair with:
  connect -- Drop action typically followed by a connect call
  setConnectionCallback -- observe resulting connection changes

Source:
  ScriptModulationMatrix.cpp  setDragCallback()
    -> WeakCallbackHolder with 3 args
    -> container->dragBroadcaster (LambdaBroadcaster<int, String, DragAction>)
