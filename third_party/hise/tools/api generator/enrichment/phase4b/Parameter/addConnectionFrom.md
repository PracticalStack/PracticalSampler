Parameter::addConnectionFrom(var connectionData) -> ScriptObject

Thread safety: UNSAFE -- modifies ValueTree with undo, performs network traversal and node lookups.
Adds or removes a modulation connection to this parameter. Two modes based on argument type:
- Object argument: creates connection from source node/parameter described in JSON.
  Returns the new Connection object, or undefined if connection cannot be created.
- Non-object argument (e.g. 0, false): removes existing connection with undo.

Required setup:
  const var nw = Synth.getExistingDspNetwork("myNetwork");
  const var nd = nw.get("gain1");
  const var p = nd.getOrCreateParameter("Gain");

Dispatch/mechanics:
  Add mode: checks Automated flag -> sets Automated=true on ValueTree
    -> resolves source as ModulationSourceNode or container parameter
    -> sourceNode.addModulationConnection(parameterId, this) -> returns Connection
  Remove mode: getConnectionSourceTree(forceRefresh) -> sets Automated=false
    -> removes connection from ValueTree with undo

Pair with:
  Connection.disconnect -- alternative way to remove from the Connection side

Anti-patterns:
  - Do NOT call with a new connection object when the parameter is already automated --
    silently returns undefined. Remove existing connection first with addConnectionFrom(0).
  - Do NOT expect an error on invalid source node ID -- silently returns undefined.
  - Do NOT create self-connections (source == target) -- silently rejected.

Source:
  NodeBase.cpp:1188  Parameter::addConnectionFrom()
    -> DragHelpers::getSourceNodeId/getSourceParameterId for parsing
    -> ModulationSourceNode::addModulationConnection() for connection creation
    -> ValueTree removal with parent->getUndoManager() for disconnect
