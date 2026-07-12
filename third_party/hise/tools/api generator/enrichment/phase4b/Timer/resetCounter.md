Timer::resetCounter() -> undefined

Thread safety: SAFE
Resets the internal millisecond counter to the current system time.
Subsequent calls to getMilliSecondsSinceCounterReset() return elapsed time
since this reset. Also called automatically by startTimer().

Pair with:
  getMilliSecondsSinceCounterReset -- reads elapsed time since last reset

Source:
  ScriptingApiObjects.cpp:5374  TimerObject::resetCounter()
    -> milliSecondCounter = Time::getMillisecondCounter()
