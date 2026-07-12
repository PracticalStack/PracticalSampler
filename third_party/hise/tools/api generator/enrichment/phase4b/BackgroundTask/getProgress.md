BackgroundTask::getProgress() -> Double

Thread safety: SAFE
Returns the current progress value (std::atomic<double>). Safe to call from
any thread including the audio thread. Returns 0.0 if no progress has been set.
Pair with:
  setProgress -- sets the value this method reads
Source:
  ScriptingApiObjects.h:556  progress is std::atomic<double> (lock-free read)
