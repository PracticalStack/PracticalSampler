Node::getChildNodes(Integer recursive) -> Array

Thread safety: UNSAFE -- creates a new Array and iterates children with heap allocations. Recursive mode traverses the full subtree.
Returns an array of child Node references. Only container nodes have children -- leaf
nodes return an empty array. When recursive is true, returns all descendants depth-first
in a flat array.
Dispatch/mechanics:
  dynamic_cast<NodeContainer*>(this) -> getNodeList()
  recursive: calls getChildNodes(true) on each child, appends results
Pair with:
  setParent -- move nodes between containers
Source:
  NodeBase.cpp  NodeBase::getChildNodes()
    -> dynamic_cast<NodeContainer*>(this) -> getNodeList()
    -> iterates children, optionally recurses depth-first
