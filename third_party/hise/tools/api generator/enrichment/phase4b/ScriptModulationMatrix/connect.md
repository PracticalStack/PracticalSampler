ScriptModulationMatrix::connect(String sourceId, String targetId, Integer addConnection) -> Integer

Thread safety: UNSAFE -- uses killVoicesAndCall to suspend audio before modifying the connection tree.
Adds or removes a modulation connection between the specified source and target.
When adding, the connection is initialized with default values based on the
target type (configurable via setMatrixModulationProperties).

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  callSuspended() -> killVoicesAndCall() suspends audio
    -> if addConnection: creates Connection ValueTree child with default properties
    -> if !addConnection: removes matching Connection child from matrixData
    -> connectionListener fires connectionCallback

Pair with:
  canConnect -- check availability before connecting
  setConnectionProperty -- configure intensity/mode after connecting
  clearAllConnections -- bulk removal alternative

Anti-patterns:
  - [BUG] Do NOT rely on the return value -- always returns false regardless of
    success. The operation is deferred via killVoicesAndCall and the result is
    discarded.
  - [BUG] Silently does nothing if sourceId is not found in the source list.
    No error is thrown. Validate with getSourceList first.

Source:
  ScriptModulationMatrix.cpp  connect()
    -> callSuspended() -> MatrixIds::Helpers::addConnection/removeConnection
    -> default values from matrixProperties.defaultInitValues or target-type inference
