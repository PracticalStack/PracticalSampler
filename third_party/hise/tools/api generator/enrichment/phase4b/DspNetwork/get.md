DspNetwork::get(String id) -> ScriptObject

Thread safety: WARNING -- string involvement (atomic ref-count operations on toString() and ID comparisons). O(n) iteration over the network's node list.
Returns a reference to the node with the given ID. If the ID matches the network's
own ID, returns the root node. If a Node object is passed instead of a string, it is
returned as-is (pass-through). Returns undefined if no node with the given ID exists.
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
Pair with:
  create -- to create a node if it does not exist yet
Source:
  DspNetwork.cpp  get()
    -> checks if id matches network ID -> returns root
    -> iterates nodes list comparing IDs
    -> also supports bracket syntax: nw["nodeId"] via AssignableObject
