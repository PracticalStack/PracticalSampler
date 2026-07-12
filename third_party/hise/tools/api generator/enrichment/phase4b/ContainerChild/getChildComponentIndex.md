ContainerChild::getChildComponentIndex(NotUndefined childIdOrComponent) -> Integer

Thread safety: UNSAFE
Returns the index of the specified child among this component's direct children.
Accepts either a string ID or a ContainerChild reference. Returns -1 if not found.
Only searches direct children -- not recursive.
Source:
  ScriptingApiContent.cpp  ChildReference::getChildComponentIndex()
    -> iterates componentData direct children
    -> matches by ID string or ValueTree identity
