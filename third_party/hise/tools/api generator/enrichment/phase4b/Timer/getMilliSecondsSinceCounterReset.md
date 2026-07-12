Timer::getMilliSecondsSinceCounterReset() -> Integer

Thread safety: SAFE
Returns milliseconds elapsed since the internal counter was last reset.
Counter is reset by resetCounter() and automatically by startTimer().

Required setup:
  const var tm = Engine.createTimerObject();
  tm.startTimer(50); // or tm.resetCounter()

Dispatch/mechanics:
  Time::getMillisecondCounter() - milliSecondCounter
  Uses uint32 system counter (wraps ~49.7 days)

Pair with:
  resetCounter -- resets the reference point for elapsed measurement
  startTimer -- automatically resets counter on start

Anti-patterns:
  - Do NOT call before startTimer() or resetCounter() -- milliSecondCounter is
    uninitialized, returns undefined elapsed value with no warning

Source:
  ScriptingApiObjects.cpp:5374  TimerObject::getMilliSecondsSinceCounterReset()
    -> Time::getMillisecondCounter() - milliSecondCounter
