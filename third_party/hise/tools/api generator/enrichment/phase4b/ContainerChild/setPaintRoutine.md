ContainerChild::setPaintRoutine(Function newPaintRoutine) -> undefined

Thread safety: UNSAFE
Registers a paint callback that draws this component's visual content. The
callback receives a Graphics object as its single argument. Inside the callback,
`this` refers to the ContainerChild. Paint execution is dispatched to the
JavaScript thread pool as a low-priority task. Triggers an immediate repaint
upon registration.
Callback signature: f(Graphics g)
Anti-patterns:
  - [BUG] Uses isValid() instead of isValidOrThrow(), unlike setControlCallback()
    and setChildCallback(). On an invalid reference, silently does nothing instead
    of throwing a script error.
  - Triggers an immediate repaint on registration -- the paint routine runs once
    automatically without needing sendRepaintMessage().
Pair with:
  sendRepaintMessage -- manually trigger a repaint after registration
  getLocalBounds -- get the component's drawing area inside the paint callback
Source:
  ScriptingApiContent.cpp  ChildReference::setPaintRoutine()
    -> creates GraphicsObject via data->createGraphicsObject(componentData, this)
    -> WeakCallbackHolder with setThisObject(this)
    -> onRefresh(RefreshType::repaint) dispatches as LowPriorityCallbackExecution
    -> graphics->getDrawHandler().flush(0, 0) commits draw actions
