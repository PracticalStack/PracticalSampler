Download::resume() -> Integer

Thread safety: SAFE -- sets a single atomic boolean flag. No allocations, no locks, no string involvement.
Requests resumption of a stopped download. Returns true if the resume request was accepted,
false if the download is running, already finished, or was aborted.

Dispatch/mechanics:
  Guard: !isRunning() && !isFinished && !shouldAbort
  Sets isWaitingForStart = true -> WebThread picks up on next iteration (up to 500ms)
  WebThread calls resumeInternal():
    -> checks existing file size, sends HTTP Range request for remaining bytes
    -> downloads to temporary sibling file, then flushTemporaryFile() appends to target

Pair with:
  stop -- must stop before resume is possible
  getStatusText -- check why resume() returned false ("Completed" vs "Aborted")
  Server.downloadFile -- use instead of resume when download was aborted or finished

Anti-patterns:
  - Do NOT call resume() after abort() -- the shouldAbort flag permanently blocks resume.
    Start a new download via Server.downloadFile() instead.
  - Do NOT assume resume() failure means "still running" -- it also returns false for
    finished or aborted downloads. Use getStatusText() to diagnose.

Source:
  ScriptingApiObjects.cpp:~1230  ScriptDownloadObject::resume()
    -> guard check: !isRunning_ && !isFinished && !shouldAbort
    -> sets isWaitingForStart = true
    -> WebThread: resumeInternal() sends Range header, downloads to temp file
