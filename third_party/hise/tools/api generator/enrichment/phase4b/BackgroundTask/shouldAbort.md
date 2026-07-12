BackgroundTask::shouldAbort() -> Integer

Thread safety: SAFE -- checks an atomic bool and extends script timeout (both lock-free). In the HISE IDE, additionally performs String-based warning logging, but backend-only code is excluded from callScope classification per the compiled-out rule.
Checks whether the background thread has been signaled to stop and extends the
script engine timeout. Returns true if sendAbortSignal() was called or a script
recompilation was triggered. Call this regularly in loops.
Dispatch/mechanics:
  engine->extendTimeout(timeout + 10) -> threadShouldExit()
  If engine is null (recompilation), signals thread exit automatically.
  HISE IDE only: warns if gap between consecutive calls exceeds timeout.
Pair with:
  sendAbortSignal -- sets the flag this method checks
  setTimeOut -- controls timeout extension amount (timeout + 10 ms per call)
Anti-patterns:
  - Do NOT omit shouldAbort() calls in background task loops -- the script engine
    timeout is not extended, risking watchdog termination. The task also becomes
    uncancellable via sendAbortSignal().
Source:
  ScriptingApiObjects.cpp  shouldAbort()
    -> #if USE_BACKEND: checks delta > timeout, logs warning
    -> engine->extendTimeout(timeOut + 10)
    -> return threadShouldExit()
