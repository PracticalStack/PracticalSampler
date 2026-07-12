Returns the number of milliseconds elapsed since the counter was last reset by `resetCounter()` or `startTimer()`. Useful for measuring durations between events independently of the timer interval.

> [!Warning:Counter uninitialised until startTimer] The counter is not initialised until `startTimer()` or `resetCounter()` is called. On a freshly created Timer, this returns an undefined value with no warning.
