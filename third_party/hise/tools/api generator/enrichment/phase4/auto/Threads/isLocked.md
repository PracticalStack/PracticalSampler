Returns `true` if the specified thread currently holds any lock. A convenience wrapper around `Threads.getLockerThread()` that checks whether the result is anything other than `Threads.Free`.
