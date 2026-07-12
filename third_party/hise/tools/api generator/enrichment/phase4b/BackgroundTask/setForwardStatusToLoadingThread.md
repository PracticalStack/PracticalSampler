BackgroundTask::setForwardStatusToLoadingThread(Integer enabled) -> undefined

Thread safety: SAFE
When enabled, setProgress() and setStatusMessage() additionally update HISE's
built-in sample loading overlay. Set before starting the task. The loading
overlay flag is automatically set when the thread starts and cleared on finish.
Pair with:
  setProgress -- progress value forwarded to SampleManager preload progress
  setStatusMessage -- message forwarded to SampleManager preload message
  callOnBackgroundThread -- start the task after enabling forwarding
Source:
  ScriptingApiObjects.cpp  setForwardStatusToLoadingThread()
    -> sets forwardToLoadingThread bool
    -> run() checks flag: getSampleManager().setPreloadFlag() / clearPreloadFlag()
