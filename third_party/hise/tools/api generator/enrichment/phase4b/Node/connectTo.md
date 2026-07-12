Node::connectTo(var parameterTarget, var sourceInfo) -> var

Thread safety: UNSAFE -- delegates to virtual addModulationConnection which modifies ValueTree structure.
Creates a connection from this node to a target parameter. Behavior depends on node type:
container nodes create macro parameter connections (sourceInfo = parameter name String),
modulation source nodes create modulation target connections (sourceInfo = output slot
index Integer). Base implementation returns undefined silently.
Dispatch/mechanics:
  casts parameterTarget to Parameter*
  -> addModulationConnection(sourceInfo, targetParameter) (virtual)
  -> NodeContainer::addMacroConnection() for containers
  -> ModulationSourceNode::addModulationConnection() for mod sources
  -> NodeBase base: returns empty var (no-op)
Pair with:
  getOrCreateParameter -- obtain the target Parameter object
  connectToBypass -- for bypass connections instead of parameter connections
Anti-patterns:
  - Silently returns undefined on leaf nodes that are not modulation sources -- no error.
  - Silently returns undefined if parameterTarget is not a valid Parameter object.
Source:
  NodeBase.cpp  NodeBase::connectTo()
    -> casts parameterTarget to Parameter*
    -> calls virtual addModulationConnection(sourceInfo, targetParameter)
    -> overrides in SerialNode/ParallelNode and ModulationSourceNode
