TransportHandler::isNonRealtime() -> Integer

Thread safety: SAFE
Returns whether the DAW is currently bouncing/exporting audio (non-realtime rendering). Returns 1 if bouncing, 0 if in realtime playback. Useful in transport callbacks to adjust processing for offline rendering.
Source:
  ScriptingApi.cpp:8714  isNonRealtime() -> SampleManager::isNonRealtime()
