ContainerChild::setChildCallback(Function newChildCallback) -> undefined

Thread safety: UNSAFE
Registers a callback that fires whenever a direct child component is added to or
removed from this component. Fires synchronously during the add/remove operation.
Inside the callback, `this` refers to the ContainerChild.
Callback signature: f(String childId, bool wasAdded)
Pair with:
  addChildComponent -- triggers the callback with wasAdded=true
  removeFromParent -- triggers the callback with wasAdded=false on the parent
  removeAllChildren -- triggers the callback for each removed child
Source:
  ScriptingApiContent.cpp  ChildReference::setChildCallback()
    -> WeakCallbackHolder with setThisObject(this)
    -> childListener on componentData for child add/remove
    -> onChildChange(v, wasAdded) passes child id and boolean flag
