BackgroundTask::setProgress(Double p) -> undefined

Thread safety: SAFE -- writes to std::atomic<double> (lock-free). When loading thread forwarding is enabled, additionally updates the SampleManager preload progress.
Sets the progress value, clamped to 0.0-1.0. Stored as atomic double, pollable
from any thread via getProgress().
Pair with:
  getProgress -- reads the value this method sets
  setForwardStatusToLoadingThread -- enables forwarding to loading overlay
Source:
  ScriptingApiObjects.cpp  setProgress()
    -> atomic store clamped to 0.0-1.0
    -> if forwardToLoadingThread: getSampleManager().getPreloadProgress() = p
