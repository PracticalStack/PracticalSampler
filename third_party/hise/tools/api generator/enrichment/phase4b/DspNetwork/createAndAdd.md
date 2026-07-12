DspNetwork::createAndAdd(String path, String id, ScriptObject parent) -> ScriptObject

Thread safety: UNSAFE -- delegates to create() (heap allocations) then calls setParent() which modifies the ValueTree hierarchy.
Convenience method that creates a node and immediately adds it as the last child of
the given parent container. Equivalent to create(path, id) followed by
node.setParent(parent, -1).
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
  const var root = nw.get(nw.getId());
Pair with:
  create -- if you need the node reference without parenting
  createFromJSON -- for building entire node trees from a descriptor
  Node.setParent -- the underlying parenting operation
Source:
  DspNetwork.cpp:840  createAndAdd()
    -> create(path, id) -> node.setParent(parent, -1)
