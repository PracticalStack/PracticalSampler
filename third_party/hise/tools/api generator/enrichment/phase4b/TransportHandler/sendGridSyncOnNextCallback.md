TransportHandler::sendGridSyncOnNextCallback() -> undefined

Thread safety: SAFE
Forces the next grid callback to have its `firstGridInPlayback` argument set to true. Provides a manual resync point for the grid, useful for resetting sequencer state. This is a global operation on the MasterClock -- affects all TransportHandler instances.
Pair with: setOnGridChange -- the callback whose `firstGridInPlayback` flag is affected.
Source:
  ScriptingApi.cpp:8704  sendGridSyncOnNextCallback() -> MasterClock::setNextGridIsFirst()
