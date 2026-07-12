Engine::setHostBpm(Number newTempo) -> undefined

Thread safety: SAFE -- writes single double member variable (globalBPM)
Overrides host BPM. Pass -1 to re-enable host sync. No validation or clamping.
Anti-patterns:
  - Passing 0 or very small values causes division-by-zero in tempo conversions
Pair with:
  getHostBpm -- read the current BPM
  createTransportHandler -- callback-based tempo tracking
Source:
  ScriptingApi.cpp  Engine::setHostBpm()
    -> GlobalSettingManager::globalBPM = newTempo
