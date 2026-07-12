ContainerChild::removeFromParent() -> undefined

Thread safety: UNSAFE
Removes this component from its parent. Execution is deferred via SafeAsyncCall.
Before removal, clears the value callback and recursively removes all descendant
values from the Values tree. Uses the parent's undo manager (not this component's).
Anti-patterns:
  - Removal is deferred -- the component is still in the tree immediately after
    this call returns.
  - Clears the value callback as a side effect. After removal, the control
    callback will not fire even if the reference is somehow still accessible.
  - Uses the parent's undo manager for removal, not this component's. Undo
    behavior depends on the parent's useUndoManager property.
Pair with:
  isValid -- check validity after removal (reference becomes permanently invalid)
Source:
  ScriptingApiContent.cpp  ChildReference::removeFromParent()
    -> SafeAsyncCall deferred execution
    -> clears valueCallback, recursively removes Values tree properties
    -> parent.removeChild(componentData, parentUm)
