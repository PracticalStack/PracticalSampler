ContainerChild::isEqual(NotUndefined other) -> Integer

Thread safety: WARNING -- String involvement when comparing by ID.
Checks whether this component matches the given argument. Accepts a string
(compared against the component's id property) or a ContainerChild reference
(compared by ValueTree identity). Returns false if the argument type is neither.
Source:
  ScriptingApiContent.cpp  ChildReference::isEqual()
    -> string: compares against componentData[dcid::id]
    -> ContainerChild: compares ValueTree identity
