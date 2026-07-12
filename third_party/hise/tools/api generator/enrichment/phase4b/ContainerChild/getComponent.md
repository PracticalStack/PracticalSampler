ContainerChild::getComponent(String childId) -> ScriptObject

Thread safety: UNSAFE
Recursively searches all descendant components and returns a ContainerChild
reference to the first component with the matching id. Returns undefined if no
match is found -- does not throw an error.
Source:
  ScriptingApiContent.cpp  ChildReference::getComponent()
    -> valuetree::Helpers::forEach recursive search
    -> parentContainer->getOrCreateChildReference(match)
