Broadcaster::attachToProcessingSpecs(var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source for audio engine spec changes (sample rate, buffer size). Broadcaster must
have 2 args (sampleRate, blockSize). No initial values on attachment.
Explicitly disables queue mode.
Dispatch/mechanics:
  Subscribes to MainController::specBroadcaster (LambdaBroadcaster<double,int>).
  Sends asynchronously. No initial values (getNumInitialCalls() = 0).
  Explicitly disables queue mode.
Pair with:
  resendLastMessage -- to get initial values after attachment
  attachToNonRealtimeChange -- related audio engine state
Anti-patterns:
  - No initial values dispatched -- use resendLastMessage if needed.
  - Explicitly sets enableQueue = false, overriding prior setEnableQueue(true).
Source:
  ScriptBroadcaster.cpp  ProcessingSpecSource constructor
