Broadcaster::callWithDelay(int delayInMilliseconds, var argArray, var function) -> undefined

Thread safety: UNSAFE -- creates DelayedFunction timer, acquires delayFunctionLock (CriticalSection)
Standalone delayed function call -- does NOT dispatch through the listener system.
Exclusive replacement: new call cancels pending one. Only one delayed function at a time.
Checks bypass state when timer fires.
Callback signature: function(var ...args)
Dispatch/mechanics:
  Standalone timer-based call -- does NOT go through listener system.
  Exclusive replacement: new call cancels any pending delayed function.
  Protected by delayFunctionLock (CriticalSection).
  Checks bypass state when timer fires.
Pair with:
  sendMessageWithDelay -- delayed send through the listener system
  addDelayedListener -- delayed listener registration
Anti-patterns:
  - Does NOT go through listener system -- addListener targets receive nothing.
  - argArray must be an array (even for single arg: [value]).
  - Non-function for function param silently does nothing.
Source:
  ScriptBroadcaster.cpp  DelayedFunction timer
