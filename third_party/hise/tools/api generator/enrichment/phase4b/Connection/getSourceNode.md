Connection::getSourceNode(int getSignalSource) -> ScriptObject

Thread safety: SAFE
Returns the source node of this connection. When getSignalSource is true, traces
through cable intermediaries (local_cable, global_cable) to the actual signal-producing
node. When false, returns the immediate source which may be a cable node.
Returns undefined if disconnected.

Dispatch/mechanics:
  getSignalSource=true: returns sourceInSignalChain (resolved at construction via findRealSource)
  getSignalSource=false: returns sourceNode (immediate parent in ValueTree hierarchy)
  findRealSource() recursively traces InterpretedCableNode -> checks parameter[0] modulation
    -> follows ModulationSourceNode chain until a non-cable node is found

Pair with:
  getTarget -- get the other end of the connection
  isConnected -- verify connection is still valid before calling

Anti-patterns:
  - Do NOT use getSignalSource=false when you need the actual signal generator --
    cable nodes act as intermediaries and are not the real source.
  - Do NOT call without checking isConnected() first if the connection may have
    been removed -- returns undefined silently.

Source:
  NodeBase.cpp:1632  ConnectionBase constructor resolves sourceNode and sourceInSignalChain
  NodeBase.cpp:2123  Helpers::findRealSource() -- recursive cable node tracing
