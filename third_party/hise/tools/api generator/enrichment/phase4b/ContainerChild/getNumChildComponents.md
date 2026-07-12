ContainerChild::getNumChildComponents() -> Integer

Thread safety: SAFE
Returns the number of direct child components. Does not count descendants
recursively. Does not check validity -- may return a stale count on an invalid
reference.
Source:
  ScriptingApiContent.cpp  ChildReference::getNumChildComponents()
    -> componentData.getNumChildren()
