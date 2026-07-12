ContainerChild::setControlCallback(Function controlCallback) -> undefined

Thread safety: UNSAFE
Registers a callback that fires when this component's value changes. Deduplicates
-- only fires when the value actually differs from the previous value. Fires
synchronously via ValueTree property listeners. Inside the callback, `this`
refers to the ContainerChild. Note: setValue() alone does NOT trigger this
callback -- call changed() afterward.
Callback signature: f(var value)
Anti-patterns:
  - The callback receives 1 argument (the value), not 2 like ScriptComponent's
    control callback (component, value). Common source of confusion.
  - The callback deduplicates: setting the same value twice in a row only fires
    the callback once.
Pair with:
  setValue + changed -- set value then trigger the callback
Source:
  ScriptingApiContent.cpp  ChildReference::setControlCallback()
    -> WeakCallbackHolder with setThisObject(this), setHighPriority()
    -> valueListener on Values tree for this component's id
    -> onValue() deduplicates: fires only if newValue != lastValue
