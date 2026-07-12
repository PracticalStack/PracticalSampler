Engine::getPreloadMessage() -> String

Thread safety: WARNING -- string construction
Returns the current sample preload status message. Empty when no preload is active.
Pair with:
  getPreloadProgress -- numeric progress (0.0-1.0)
  setPreloadMessage -- set custom loading message
Source:
  ScriptingApi.cpp  Engine::getPreloadMessage()
    -> SampleManager::getPreloadMessage()
