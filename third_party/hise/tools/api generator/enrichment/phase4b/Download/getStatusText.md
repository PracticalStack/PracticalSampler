Download::getStatusText() -> String

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns a human-readable string describing the current download state. Checks atomic
flags in priority order: isRunning_ > shouldAbort > isFinished > isWaitingForStop.

Values:
  "Downloading"  -- actively transferring data
  "Aborted"      -- cancelled via abort()
  "Completed"    -- finished (check data.success to distinguish success from failure)
  "Paused"       -- stopped via stop(), can be resumed
  "Waiting"      -- queued but not yet started by WebThread

Dispatch/mechanics:
  Sequential flag check: isRunning_ first (highest priority), then shouldAbort,
  isFinished, isWaitingForStop, else "Waiting"

Anti-patterns:
  - Do NOT treat "Completed" as success -- connection failures also produce "Completed"
    with data.success = false. Always check data.success alongside the status text.

Source:
  ScriptingApiObjects.cpp:~1280  ScriptDownloadObject::getStatusText()
    -> if (isRunning_) return "Downloading"
    -> if (shouldAbort) return "Aborted"
    -> if (isFinished) return "Completed"
    -> if (isWaitingForStop) return "Paused"
    -> return "Waiting"
