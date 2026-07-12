RoutingMatrix::removeSendConnection(Number sourceIndex, Number destinationIndex) -> Integer

Thread safety: UNSAFE -- acquires SimpleReadWriteLock write lock; triggers sendChangeMessage.
Removes the send connection between sourceIndex and destinationIndex.
Returns true if the send connection existed and was removed.
Dispatch/mechanics:
  MatrixData::removeSendConnection() acquires write lock
    -> sets sendConnections[source] = -1
    -> refreshSourceUseStates() -> sendChangeMessage()
Pair with:
  addSendConnection -- add send connections
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::removeSendConnection()
    -> MatrixData send connection removal in Routing.cpp
    -> sendConnections[source] = -1, refreshSourceUseStates()
