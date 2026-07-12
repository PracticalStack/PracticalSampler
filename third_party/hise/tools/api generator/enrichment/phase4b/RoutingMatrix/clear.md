RoutingMatrix::clear() -> undefined

Thread safety: UNSAFE -- acquires SimpleReadWriteLock write lock multiple times (resetToDefault + two removeConnection calls); triggers sendChangeMessage.
Removes all primary and send connections. Internally calls resetToDefault() (which sets stereo passthrough 0->0, 1->1) then explicitly removes those default connections.
Dispatch/mechanics:
  resetToDefault() clears all arrays, sets channelConnections[0]=0, [1]=1
    -> removeConnection(0,0) -> removeConnection(1,1)
    -> with numAllowedConnections == 2, auto-correction may re-add a default passthrough
Pair with:
  addConnection -- rebuild connections after clearing
  setNumChannels -- relax stereo constraint before clear to ensure truly empty matrix
Anti-patterns:
  - Do NOT assume clear() produces an empty matrix under the default stereo constraint --
    numAllowedConnections == 2 auto-restores a passthrough connection. Call setNumChannels()
    first to relax the constraint if you need a truly empty matrix.
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::clear()
    -> MatrixData::resetToDefault() in Routing.cpp
    -> MatrixData::removeConnection(0,0) + removeConnection(1,1)
