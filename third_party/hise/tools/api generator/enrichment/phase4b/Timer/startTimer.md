Timer::startTimer(int intervalInMilliSeconds) -> undefined

Thread safety: UNSAFE -- JUCE Timer::startTimer acquires a lock on the internal timer thread
Starts the timer with the given interval in milliseconds. Minimum interval is
11ms -- values <= 10 throw "Go easy on the timer". If already running, changes
the interval. Automatically calls resetCounter().

Pair with:
  setTimerCallback -- must be set before starting (timer auto-stops without valid callback)
  stopTimer -- stops the timer
  isTimerRunning -- check if already running

Anti-patterns:
  - Do NOT pass intervals <= 10 -- throws a script error
  - Do NOT rely on getMilliSecondsSinceCounterReset() across startTimer calls --
    startTimer resets the counter, silently invalidating elapsed time measurements

Source:
  ScriptingApiObjects.cpp:5374  TimerObject::startTimer()
    -> if (interval > 10): it.startTimer(interval) + resetCounter()
    -> else: throw String("Go easy on the timer")
