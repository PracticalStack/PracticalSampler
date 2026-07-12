ScriptDynamicContainer::setValueCallback(Function valueFunction) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder and sets up a ValueTree listener on the data model's Values tree.
Registers a callback that fires whenever any dyncomp child component's value
changes. Synchronous mode, high priority. Requires setData() to have been called
first.
Callback signature: f(String componentId, var newValue)
Required setup:
  const var dc = Content.addDynamicContainer("Container1", 0, 0);
  dc.setPosition(0, 0, 300, 100);
  const var root = dc.setData([{"id": "Vol", "type": "Slider"}]);
Dispatch/mechanics:
  Creates WeakCallbackHolder(2 args) -> sets high priority
    -> listens to data->getValueTree(Values) with AnyPropertyListener
    -> on change: calls callback(id.toString(), newValue) synchronously
Anti-patterns:
  - Do NOT call before setData() -- silently does nothing because the Values tree
    does not exist yet. No error reported.
Pair with:
  setData -- must be called first to create the data model
  setControlCallback -- separate system for the container's own value
Source:
  ScriptingApiContent.cpp  ScriptDynamicContainer::setValueCallback()
    -> checks data != nullptr -> new WeakCallbackHolder
    -> valueListener.setCallback(ValuesTree, Synchronously)
