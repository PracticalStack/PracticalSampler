Download::getDownloadSize() -> Double

Thread safety: SAFE -- reads two int64 member variables and returns their sum as a double. No allocations, no locks.
Returns the total download size in bytes, including pre-existing bytes from a previous
partial download when resuming. Returns 0.0 before the download starts or if the server
does not report a content length.

Pair with:
  getNumBytesDownloaded -- current bytes downloaded (same accounting for resume offset)
  getProgress -- ratio of downloaded/total

Source:
  ScriptingApiObjects.cpp:~1310  ScriptDownloadObject::getDownloadSize()
    -> returns (double)(totalLength_ + existingBytesBeforeResuming)
