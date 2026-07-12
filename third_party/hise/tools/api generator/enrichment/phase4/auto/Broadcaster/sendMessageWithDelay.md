Sends a message to all listeners after a specified delay in milliseconds. Each new call replaces the previous pending message and restarts the timer, making this suitable for debouncing. Only the last call's arguments are dispatched.

If `setForceSynchronousExecution(true)` has been called, the delay is bypassed entirely and the message fires synchronously and immediately.
