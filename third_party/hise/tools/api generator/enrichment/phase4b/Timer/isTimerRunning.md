Timer::isTimerRunning() -> Integer

Thread safety: SAFE
Returns whether the timer is currently running. Reads an internal flag.

Source:
  ScriptingApiObjects.cpp:5374  TimerObject::isTimerRunning()
    -> InternalTimer::isTimerRunning() (JUCE Timer member flag)
