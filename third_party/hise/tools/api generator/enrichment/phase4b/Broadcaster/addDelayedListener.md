Broadcaster::addDelayedListener(int delayInMilliSeconds, var obj, var metadata, var function) -> Integer

Thread safety: UNSAFE -- allocates OwnedArray entries, creates timer objects for deferred execution
Adds a debounced listener. Each broadcast restarts the delay timer -- only the last message
within the delay window fires the callback. If delay is 0, falls back to addListener().
Callback uses broadcaster's lastValues at timer-fire time, not the values from callSync.
Callback signature: function(var ...broadcastArgs)
Dispatch/mechanics:
  If delay == 0, falls back to addListener().
  callSync() creates new DelayedFunction timer each time, replacing previous.
  Timer fires using parent->lastValues (not args from callSync time).
Pair with:
  addListener -- for immediate (non-delayed) callbacks
  removeListener -- remove by metadata
Anti-patterns:
  - Callback receives lastValues at timer-fire time, not values that triggered the delay.
  - Each new broadcast replaces the pending delayed call -- no accumulation.
  - Callback is bypassed if broadcaster is in bypassed state when timer fires.
Source:
  ScriptBroadcaster.cpp:902  DelayedItem constructor
