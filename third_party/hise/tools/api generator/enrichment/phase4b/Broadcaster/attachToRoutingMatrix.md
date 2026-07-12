Broadcaster::attachToRoutingMatrix(var moduleIds, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source for routing matrix changes. Broadcaster must have 2 args (processorId,
matrix). The matrix arg receives a ScriptRoutingMatrix object. Auto-enables queue mode.
Provides initial values per processor.
Dispatch/mechanics:
  Registers SafeChangeListener on RoutableProcessor::getMatrix().
  Sends (processorId, ScriptRoutingMatrix) asynchronously.
  Auto-enables queue mode. Provides initial values per processor.
Pair with:
  addListener -- to handle matrix change events
Anti-patterns:
  - Module must have a routing matrix (RoutableProcessor) -- modulators do not.
  - Queue mode forced on.
  - Must not call functions in listener callback that cause routing matrix changes -- creates infinite loop.
Source:
  ScriptBroadcaster.cpp  RoutingMatrixListener constructor
