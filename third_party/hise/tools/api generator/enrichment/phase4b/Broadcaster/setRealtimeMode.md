Broadcaster::setRealtimeMode(bool enableRealTimeMode) -> undefined

Thread safety: SAFE
Enables lock-free realtime-safe dispatch. When true: sendMessageInternal skips write lock,
sendInternal skips read lock and copies, isRealtimeSafe() returns true.
Required for audio-thread synchronous sends. Listener callbacks validated for rt-safety.
Auto-enabled by attachToNonRealtimeChange.
Dispatch/mechanics:
  Sets realtimeSafe bool. When true:
  sendMessageInternal skips write lock, value comparison, async path.
  sendInternal skips read lock, iterates items directly without copying args.
Pair with:
  setForceSynchronousExecution -- often used together for audio-thread dispatch
  attachToNonRealtimeChange -- auto-enables this
Anti-patterns:
  - Removes thread-safety locks -- assumes listener list is stable (modified only at init).
  - Adding listeners after broadcaster is in use with realtime mode may cause data races.
Source:
  ScriptBroadcaster.cpp  setRealtimeMode()
