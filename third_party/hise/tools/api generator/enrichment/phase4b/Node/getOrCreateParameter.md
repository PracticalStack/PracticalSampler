Node::getOrCreateParameter(var indexOrId) -> ScriptObject

Thread safety: UNSAFE -- when creating: modifies ValueTree structure with undo manager. When retrieving: performs tree search with string comparison.
Retrieves an existing parameter by name, index, or JSON descriptor. If not found and
the node is a container, creates a new parameter from the JSON descriptor. Leaf nodes
report a script error on creation attempts.
Required setup:
  const var nd = nw.get("myContainer"); // must be a container node
Dispatch/mechanics:
  getParameter(indexOrId) -> returns existing if found
  if not found and NodeContainer: creates Parameter ValueTree with range/mode properties
  if not a container: script error "Can't create parameter for non-container node"
Pair with:
  getNumParameters -- check parameter count before lookup
  connectTo -- wire a macro parameter to a target
Anti-patterns:
  - Do NOT call on leaf nodes expecting parameter creation -- only containers support it.
    Leaf nodes produce a script error.
Source:
  NodeBase.cpp  NodeBase::getOrCreateParameter()
    -> getParameter(indexOrId) first
    -> if NodeContainer: creates new Parameter ValueTree, adds to Parameters child tree
