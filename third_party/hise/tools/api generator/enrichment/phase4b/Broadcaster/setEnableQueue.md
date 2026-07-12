Broadcaster::setEnableQueue(bool shouldUseQueue) -> undefined

Thread safety: SAFE
Enables/disables queue mode. When enabled: (1) change detection bypassed -- identical values
still dispatch, (2) async coalescing bypassed -- every send posts independent job.
Auto-enabled by attachToModuleParameter, attachToRoutingMatrix, attachToContextMenu,
attachToComplexData (multi), attachToSampleMap (multi).
Pair with:
  sendAsyncMessage -- most affected by queue mode
  setForceSynchronousExecution -- related dispatch configuration
Notes:
  Without queue: bc.sendAsyncMessage([0]); bc.sendAsyncMessage([1]); -- value 0 never reaches listeners (coalesced to 1).
  With queue: both values delivered in order.
Anti-patterns:
  - Queue mode with frequent async sends may cause large job backlog on JavascriptThreadPool.
  - Disabling queue after auto-enable can cause silent message loss.
Source:
  ScriptBroadcaster.cpp  setEnableQueue()
