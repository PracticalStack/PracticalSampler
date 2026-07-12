TransportHandler::getGridPosition(Integer timestamp) -> Integer

Thread safety: SAFE
Returns the current PPQ position as an integer for the given sample timestamp offset within the current audio block (0 = start of block). The PPQ value is truncated to an integer.
Pair with: setOnGridChange -- provides the grid index contextually during grid callbacks.
Source:
  ScriptingApi.cpp:8734  getGridPosition() -> MasterClock::getPPQPos(timestamp) -> truncated to int
