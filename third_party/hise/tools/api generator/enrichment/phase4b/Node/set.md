Node::set(var id, var value) -> undefined

Thread safety: UNSAFE -- writes ValueTree properties via undo manager, involving heap allocations and notification chains.
Sets a property value on the node. Checks two locations independently: node properties
(Properties child tree) and direct ValueTree properties (Bypassed, NodeColour, Comment,
Folded). Silently does nothing if the ID is not found in either location.
Dispatch/mechanics:
  hasNodeProperty(id) -> setNodeProperty(id, value) via PropertyTree
  getValueTree().hasProperty(id) -> setProperty(id, value) via UndoManager
  Both locations checked independently; both can match for the same call.
Pair with:
  get -- reads back node properties (but not ValueTree properties)
Anti-patterns:
  - Silently does nothing for unknown property IDs -- no error reported.
  - get()/set() asymmetry: set("NodeColour", 0xFF0000) succeeds, but get("NodeColour")
    returns undefined because get() only checks node properties.
Source:
  NodeBase.cpp  NodeBase::set()
    -> hasNodeProperty(id) -> setNodeProperty(id, value)
    -> getValueTree().hasProperty(id) -> v_data.setProperty(id, value, getUndoManager())
