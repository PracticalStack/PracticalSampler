RoutingMatrix::addConnection(Number sourceIndex, Number destinationIndex) -> Integer

Thread safety: UNSAFE -- acquires SimpleReadWriteLock write lock on MatrixData; triggers sendChangeMessage for listener notification.
Adds a primary channel connection routing audio from sourceIndex to destinationIndex.
Returns true if the connection was successfully added. Each source maps to at most one destination; calling again on the same source overwrites the previous destination.
Required setup:
  const var rm = Synth.getRoutingMatrix("SynthName");
Dispatch/mechanics:
  MatrixData::addConnection() acquires write lock
    -> sets channelConnections[source] = destination
    -> if numAllowedConnections == 2 and count > 2, removes oldest even/odd connection
    -> refreshSourceUseStates() -> sendChangeMessage()
Pair with:
  removeConnection -- remove a previously added connection
  setNumChannels -- relax stereo constraint before multichannel routing
  clear -- reset all connections before rebuilding
Anti-patterns:
  - Do NOT add multichannel connections without calling setNumChannels() first -- the default
    stereo constraint (numAllowedConnections == 2) auto-removes connections when a third is added
  - Do NOT ignore the return value when routing to higher output pairs -- the host may not
    support the requested channel count, and the method returns false on failure
Source:
  ScriptingApiObjects.cpp:3959  ScriptRoutingMatrix::addConnection()
    -> MatrixData::addConnection() in Routing.cpp
    -> channelConnections[source] = destination, refreshSourceUseStates()
