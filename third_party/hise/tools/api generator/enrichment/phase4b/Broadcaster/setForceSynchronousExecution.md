Broadcaster::setForceSynchronousExecution(bool shouldExecuteSynchronously) -> undefined

Thread safety: SAFE
Forces all sends to execute synchronously regardless of the send method used.
sendAsyncMessage becomes sync, sendMessageWithDelay skips delay, resendLastMessage
forced to sync. Auto-enabled by addModuleParameterSyncer.
Pair with:
  setRealtimeMode -- required if receiving audio-thread events with forceSync
  addModuleParameterSyncer -- auto-enables this
Anti-patterns:
  - Expensive callbacks block the calling thread (which may be audio thread if realtime mode is on).
Source:
  ScriptBroadcaster.cpp  setForceSynchronousExecution()
