Engine::getHostBpm() -> Double

Thread safety: SAFE -- reads atomic<double> (lock-free)
Returns current BPM from host or manual override. Returns 120.0 if no BPM set.
Pair with:
  setHostBpm -- override or re-enable host sync (-1)
  createTransportHandler -- callback-based tempo tracking
Source:
  ScriptingApi.cpp  Engine::getHostBpm()
    -> MainController::getBpm() (atomic read, defaults to 120.0 if <= 0)
