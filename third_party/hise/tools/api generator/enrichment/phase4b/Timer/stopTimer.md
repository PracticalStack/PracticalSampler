Timer::stopTimer() -> undefined

Thread safety: UNSAFE -- JUCE Timer::stopTimer acquires a lock on the internal timer thread
Stops the timer. No further callbacks fire after this call.
Safe no-op if called on a timer that is not running. Also stops automatically
on object destruction, MainController shutdown, or invalid callback.

Source:
  ScriptingApiObjects.cpp:5374  TimerObject::stopTimer()
    -> it.stopTimer() (JUCE Timer)
