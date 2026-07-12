ContainerChild::getParent() -> ScriptObject

Thread safety: UNSAFE
Returns a ContainerChild reference to this component's parent in the data tree.
On the root component returned by setData(), returns a reference to the data
tree root node itself.
Source:
  ScriptingApiContent.cpp  ChildReference::getParent()
    -> parentContainer->getOrCreateChildReference(componentData.getParent())
