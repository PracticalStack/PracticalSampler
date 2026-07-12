BackgroundTask::setTimeOut(Integer newTimeout) -> undefined

Thread safety: SAFE
Sets the timeout in milliseconds for thread stop operations. Affects:
sendAbortSignal(true) blocking duration, stopThread() wait when starting a new
task, and the interval added to the script engine timeout by shouldAbort().
Default: 500 ms.
Pair with:
  shouldAbort -- extends script timeout by timeout+10 ms per call
  sendAbortSignal -- uses timeout for blocking wait duration
  callOnBackgroundThread -- uses timeout when stopping previous task
Source:
  ScriptingApiObjects.cpp  setTimeOut()
    -> sets timeOut member (int, default 500)
