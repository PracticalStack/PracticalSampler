ContainerChild::isValid() -> Integer

Thread safety: WARNING -- May modify internal invalidation state as a side effect.
Checks whether this reference still points to a valid component within the
container's data tree. Returns false if invalidated (e.g., by setData() or
removeFromParent()). Once invalidity is detected, the reference permanently
disconnects from the refresh broadcaster -- one-way transition.
Anti-patterns:
  - Has a side effect: when invalidity is detected, disconnects the refresh
    broadcaster listener and sets the invalid flag permanently. Not a pure query.
Source:
  ScriptingApiContent.h:2396+  ChildReference::isValid()
    -> checks invalid flag and parentContainer WeakReference
    -> valuetree::Helpers::isParent(componentData, dataTree)
    -> on failure: refreshBroadcaster.removeListener(), invalid = true
