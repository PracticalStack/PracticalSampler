Download::isRunning() -> Integer

Thread safety: SAFE -- reads a single atomic boolean. No allocations, no locks, no string involvement.
Returns true if the download is actively transferring data, false otherwise.
A download in "Waiting" or "Paused" state returns false.

Source:
  ScriptingApiObjects.cpp:~1270  ScriptDownloadObject::isRunning()
    -> returns (bool)isRunning_ atomic flag
