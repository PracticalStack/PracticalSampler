RoutingMatrix::addSendConnection(Number sourceIndex, Number destinationIndex) -> Integer

Thread safety: UNSAFE -- acquires SimpleReadWriteLock write lock on MatrixData; triggers sendChangeMessage.
Adds a send connection routing a copy of audio from sourceIndex to destinationIndex in the parallel send bus.
Send connections operate independently of primary connections, allowing the same source to feed both a main destination and a send destination simultaneously.
Required setup:
  const var rm = Synth.getRoutingMatrix("SynthName");
Dispatch/mechanics:
  MatrixData::addSendConnection() acquires write lock
    -> sets sendConnections[source] = destination
    -> refreshSourceUseStates() -> sendChangeMessage()
Pair with:
  removeSendConnection -- remove a previously added send connection
  addConnection -- primary routing (independent of send bus)
Source:
  ScriptingApiObjects.cpp:3959  ScriptRoutingMatrix::addSendConnection()
    -> MatrixData::toggleSendEnabling() in Routing.cpp
    -> sendConnections[source] = destination, refreshSourceUseStates()
