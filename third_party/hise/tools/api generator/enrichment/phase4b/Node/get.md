Node::get(var id) -> var

Thread safety: WARNING -- id.toString() and Identifier construction involve atomic ref-count operations.
Returns the value of a node-type-specific property from the Properties child tree.
Only reads node properties (e.g. "Mode", "Frequency") -- not ValueTree properties like Bypassed or NodeColour. Returns undefined if property ID not found.
Dispatch/mechanics:
  getPropertyTree().getChildWithProperty(PropertyIds::ID, id.toString())
  -> returns child[PropertyIds::Value] or undefined
Pair with:
  set -- writes to the same node properties (and also ValueTree properties)
Anti-patterns:
  - Do NOT use get() to read ValueTree properties like Bypassed or NodeColour -- returns
    undefined. Use isBypassed() for bypass state.
Source:
  NodeBase.cpp  NodeBase::get()
    -> getPropertyTree().getChildWithProperty(PropertyIds::ID, id.toString())
    -> returns child[PropertyIds::Value]
