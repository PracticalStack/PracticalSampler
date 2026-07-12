DspNetwork::createFromJSON(JSON jsonData, ScriptObject parent) -> ScriptObject

Thread safety: UNSAFE -- recursively creates nodes via createAndAdd(), involving heap allocations, ValueTree construction, and string operations.
Recursively creates a tree of nodes from a JSON object and adds them to the given
parent container. Each JSON object must have FactoryPath and ID properties. If the
object has a Nodes array, its elements are recursively processed as child nodes.
Returns the top-level created node, or undefined if creation fails.
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
  const var root = nw.get(nw.getId());
Pair with:
  create -- for creating individual nodes without JSON
  createAndAdd -- the underlying per-node operation
Source:
  DspNetwork.cpp:858  createFromJSON()
    -> reads FactoryPath, ID from each JSON object
    -> calls createAndAdd(path, id, parent) for each
    -> recursively processes Nodes array for child nodes
