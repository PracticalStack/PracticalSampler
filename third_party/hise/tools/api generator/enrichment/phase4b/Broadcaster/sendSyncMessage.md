Broadcaster::sendSyncMessage(var args) -> undefined

Thread safety: WARNING -- safe on scripting/UI thread. Audio thread requires setRealtimeMode(true), otherwise backend throws error. Acquires lastValueLock in non-realtime mode.
Sends synchronous message to all listeners via sendMessageInternal(args, true).
Change detection suppresses duplicate values. For single-arg broadcasters, pass value
directly (not wrapped in array). Equivalent to dot-assignment but sets all args at once.
Dispatch/mechanics:
  sendMessageInternal -> change detection -> write lock on lastValues
  -> sendInternal() iterates targets, calls callSyncWithProfile() on each.
  Realtime-safe path: no locks, direct iteration.
  Backend safety check: audio thread + sync + !realtimeSafe = error.
Pair with:
  sendAsyncMessage -- async alternative
  setRealtimeMode -- required for audio-thread sends
  setEnableQueue -- to bypass change detection
Anti-patterns:
  - Duplicate values silently suppressed unless queue mode enabled.
  - Audio thread + sync + !realtimeSafe = script error in backend, undefined behavior in frontend.
Source:
  ScriptBroadcaster.cpp:3662  sendMessageInternal(args, true)
