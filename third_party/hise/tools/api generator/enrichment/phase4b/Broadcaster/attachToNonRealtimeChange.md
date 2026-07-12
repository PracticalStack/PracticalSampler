Broadcaster::attachToNonRealtimeChange(var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires when audio engine switches realtime/non-realtime mode.
Broadcaster must have 1 arg (isNonRealtime). Sends synchronously.
Auto-enables realtime mode on the broadcaster -- all subsequent listeners must be realtime-safe.
Dispatch/mechanics:
  Subscribes to MainController::realtimeBroadcaster (LambdaBroadcaster<bool>).
  Sends synchronously via sendSyncMessage (critical for audio path decisions).
  Auto-enables realtime mode on the broadcaster.
Pair with:
  setRealtimeMode -- auto-enabled by this method
  attachToProcessingSpecs -- related audio engine state
Anti-patterns:
  - Forces realtime mode on broadcaster -- listeners must be inline functions in exported plugins.
  - Synchronous dispatch: callback executes on whatever thread triggers the change (may be audio thread).
Source:
  ScriptBroadcaster.cpp  NonRealtimeSource constructor
