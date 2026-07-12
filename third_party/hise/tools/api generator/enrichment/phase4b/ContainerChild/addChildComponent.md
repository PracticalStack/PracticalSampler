ContainerChild::addChildComponent(JSON childData) -> ScriptObject

Thread safety: UNSAFE
Creates a new child component from JSON data and appends it as the last child.
Returns a ContainerChild reference to the new child. Position via bounds array
[x, y, w, h] or individual x, y, width, height properties (defaults: 0, 0, 128, 50).
Required setup:
  const var dc = Content.addDynamicContainer("DC1", 0, 0);
  const var cc = dc.setData({"id": "Root", "type": "Container"});
Dispatch/mechanics:
  dyncomp::Data::fromJSON(childData, bounds) -> ValueTree
    -> componentData.addChild(v, -1, um)
    -> parentContainer->getOrCreateChildReference(v)
Pair with:
  removeFromParent -- to remove a child added this way
  getComponent -- to retrieve the child later by ID
Anti-patterns:
  - No API to insert at a specific index -- children are always appended at the end
Source:
  ScriptingApiContent.cpp:6171+  ChildReference::addChildComponent()
    -> dyncomp::Data::fromJSON(childData, bounds)
    -> componentData.addChild(v, -1, um)
    -> parentContainer->getOrCreateChildReference(v)
