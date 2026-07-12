Starts the timer with the given interval in milliseconds. If the timer is already running, this changes the interval. The minimum interval is 11ms.

> [!Warning:Resets internal millisecond counter] Calling `startTimer` automatically resets the internal millisecond counter. If you use `getMilliSecondsSinceCounterReset()` to measure elapsed time independently, restarting the timer silently zeroes your measurement.
