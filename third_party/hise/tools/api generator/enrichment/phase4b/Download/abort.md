Download::abort() -> Integer

Thread safety: SAFE -- sets two atomic boolean flags only, no allocations, no locks.
Aborts the download, marks it for cancellation, and deletes the target file.
Returns true if the download was running at the time of the call, false if already stopped or finished.

Dispatch/mechanics:
  Sets shouldAbort = true, then calls stop() (sets isWaitingForStop = true)
    -> WebThread calls stopInternal(): nulls DownloadTask, deletes target file,
       sets data.aborted = true, data.finished = true, data.success = false, fires callback

Pair with:
  getStatusText -- returns "Aborted" after abort completes
  stop -- pauses without deleting (use when you want to resume later)

Anti-patterns:
  - Do NOT call resume() after abort() -- the shouldAbort flag blocks resume permanently.
    Start a new download via Server.downloadFile() instead.
  - Do NOT check state immediately after abort() returns -- abort is asynchronous.
    The file deletion and status change happen on the WebThread (up to 500ms later).

Source:
  ScriptingApiObjects.cpp:~1200  ScriptDownloadObject::abort()
    -> sets shouldAbort = true
    -> calls stop() -> isWaitingForStop = true
    -> WebThread: stopInternal() deletes targetFile, sets data properties, fires callback
