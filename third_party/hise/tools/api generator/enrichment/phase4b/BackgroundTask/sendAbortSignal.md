BackgroundTask::sendAbortSignal(Integer blockUntilStopped) -> undefined

Thread safety: UNSAFE -- when blockUntilStopped is true, calls stopThread() which blocks the calling thread for up to timeout ms and extends the script engine timeout. Non-blocking mode only sets an atomic flag.
Signals the background thread to stop. When blockUntilStopped is false, sets
the thread exit flag (non-blocking). When true, blocks until the thread exits
or the timeout expires. Does nothing if no thread is running.
Dispatch/mechanics:
  blockUntilStopped=false: signalThreadShouldExit() (atomic flag set)
  blockUntilStopped=true: extendTimeout(timeout+10) -> stopThread(timeout)
Pair with:
  shouldAbort -- returns true after sendAbortSignal is called
  setTimeOut -- controls blocking duration when blockUntilStopped is true
Anti-patterns:
  - Do NOT call with blockUntilStopped=true from inside the background task function --
    causes deadlock (thread waiting for itself). Detected and throws script error:
    "Can't stop with blocking on the worker thread"
Source:
  ScriptingApiObjects.cpp  sendAbortSignal()
    -> if blockUntilStopped && getCurrentThread()==this: reportScriptError()
    -> else if blockUntilStopped: extendTimeout() + stopThread(timeout)
    -> else: signalThreadShouldExit()
