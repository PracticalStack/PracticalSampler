Download::stop() -> Integer

Thread safety: SAFE -- sets a single atomic boolean flag. No allocations, no locks, no string involvement.
Requests that the download be paused. Returns true if the download was actively running,
false if it was not running (already stopped, waiting, finished, or aborted).

Dispatch/mechanics:
  Sets isWaitingForStop = true -> WebThread calls stopInternal() on next iteration (up to 500ms)
  stopInternal(): nulls DownloadTask, flushes temp resume data to target,
    sets data.finished = true, data.success = false, fires callback
  Target file is preserved (unlike abort which deletes it)

Pair with:
  resume -- to continue the download after stopping
  abort -- to cancel permanently and delete the target file

Anti-patterns:
  - Do NOT check isRunning() immediately after stop() returns true and expect false --
    stop is asynchronous. The actual stop happens on the WebThread (up to 500ms later).

Source:
  ScriptingApiObjects.cpp:~1210  ScriptDownloadObject::stop()
    -> sets isWaitingForStop = true
    -> WebThread: stopInternal() nulls download task, sets data, fires callback
