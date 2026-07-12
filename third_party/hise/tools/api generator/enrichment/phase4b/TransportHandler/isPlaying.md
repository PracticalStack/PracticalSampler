TransportHandler::isPlaying() -> Integer

Thread safety: SAFE
Returns whether the transport is currently playing (internal or external clock, depending on sync mode). Returns 1 if playing, 0 if stopped.
Pair with: setOnTransportChange -- receive play state changes as callbacks instead of polling.
Source:
  ScriptingApi.cpp:8729  isPlaying() -> MasterClock::isPlaying()
