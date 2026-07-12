ContainerChild::removeAllChildren() -> undefined

Thread safety: UNSAFE
Removes all child components from this component. Execution is deferred via
SafeAsyncCall -- the children are not removed immediately upon return. Uses
this component's undo manager if useUndoManager is true.
Anti-patterns:
  - Removal is deferred -- getNumChildComponents() may still return the old
    count immediately after this call.
Pair with:
  addChildComponent -- to rebuild the child list after clearing
Source:
  ScriptingApiContent.cpp  ChildReference::removeAllChildren()
    -> SafeAsyncCall deferred execution
    -> componentData.removeAllChildren(um)
