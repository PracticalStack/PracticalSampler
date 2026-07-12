Signals the background thread to stop. With `blockUntilStopped` set to false, this is non-blocking - it sets a flag so that `shouldAbort()` returns true on the next check. With `blockUntilStopped` set to true, the calling thread blocks until the background thread exits or the timeout expires.

> [!Warning:Blocking from background task causes deadlock] Calling with `blockUntilStopped = true` from within the background task function causes a deadlock (the thread waits for itself to stop). This is detected and throws a script error.
