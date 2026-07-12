RoutingMatrix::removeConnection(Number sourceIndex, Number destinationIndex) -> Integer

Thread safety: UNSAFE -- acquires SimpleReadWriteLock write lock; triggers sendChangeMessage.
Removes the primary channel connection between sourceIndex and destinationIndex.
Returns true if the connection existed and was removed.
Dispatch/mechanics:
  MatrixData::removeConnection() acquires write lock
    -> sets channelConnections[source] = -1
    -> if numAllowedConnections == 2 and count drops below 2, auto-restores default passthrough
    -> refreshSourceUseStates() -> sendChangeMessage()
Pair with:
  addConnection -- add connections back after removal
  clear -- remove all connections at once
Anti-patterns:
  - Do NOT expect removal to leave a gap under the default stereo constraint -- when
    numAllowedConnections == 2, removing a connection that drops count below 2 auto-restores
    a default passthrough (channelConnections[index] = index).
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::removeConnection()
    -> MatrixData::removeConnection() in Routing.cpp
    -> channelConnections[source] = -1, refreshSourceUseStates()
