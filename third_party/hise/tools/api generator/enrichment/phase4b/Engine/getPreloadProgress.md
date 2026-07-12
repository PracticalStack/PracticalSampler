Engine::getPreloadProgress() -> Double

Thread safety: SAFE -- reads numeric value
Returns sample preload progress (0.0 to 1.0). 0.0 when no preload active.
Pair with:
  getPreloadMessage -- human-readable status text
Source:
  ScriptingApi.cpp  Engine::getPreloadProgress()
    -> SampleManager::getPreloadProgress()
