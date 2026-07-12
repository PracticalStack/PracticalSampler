ContainerChild::sendRepaintMessage(Integer recursive) -> undefined

Thread safety: UNSAFE
Sends a repaint message to this component. If a paint routine is registered, it
will be re-executed on the JavaScript thread pool. When recursive is true, the
message propagates to all descendant components.
Pair with:
  setPaintRoutine -- registers the paint callback that repaint triggers
Source:
  ScriptingApiContent.cpp  ChildReference::sendRepaintMessage()
    -> sendMessage(RefreshType::repaint, recursive)
    -> dispatched via refreshBroadcaster
    -> paint executes as LowPriorityCallbackExecution on JS thread pool
