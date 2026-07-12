Broadcaster::sendMessageWithDelay(var args, int delayInMilliseconds) -> undefined

Thread safety: UNSAFE -- starts Timer, stores pendingData, deferred send dispatches asynchronously
Sends message after a delay. Exclusive replacement: new call replaces pending data and
restarts timer. Deferred send is always async. If forceSync is true, delay is bypassed
entirely and message fires synchronously and immediately.
Dispatch/mechanics:
  Stores pendingData, starts Timer with delay.
  Timer fires -> sendMessage(pendingData, false) (async).
  Exclusive replacement: new call replaces pending data and restarts timer.
  If forceSync is true, delay is bypassed entirely.
Pair with:
  sendAsyncMessage -- immediate async send
  callWithDelay -- standalone delayed function call (not through listener system)
Anti-patterns:
  - Deferred send always async -- forceSync overrides by skipping delay entirely.
  - Multiple calls before timer fires replace pending data -- only last values sent.
Source:
  ScriptBroadcaster.cpp  timerCallback()
