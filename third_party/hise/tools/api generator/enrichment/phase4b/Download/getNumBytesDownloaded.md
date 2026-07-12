Download::getNumBytesDownloaded() -> Double

Thread safety: SAFE -- reads two int64 member variables and returns their sum as a double. No allocations, no locks.
Returns the number of bytes downloaded so far, including pre-existing bytes from a
previous partial download when resuming. Returns 0.0 before the download starts.

Pair with:
  getDownloadSize -- total expected bytes (same accounting for resume offset)
  getProgress -- ratio of downloaded/total

Source:
  ScriptingApiObjects.cpp:~1340  ScriptDownloadObject::getNumBytesDownloaded()
    -> returns (double)(bytesDownloaded_ + existingBytesBeforeResuming)
