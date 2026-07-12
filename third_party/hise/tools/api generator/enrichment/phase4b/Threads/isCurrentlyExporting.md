Threads::isCurrentlyExporting() -> Integer

Thread safety: SAFE
Returns true if the audio export thread is active (offline rendering in
progress). Use for branching between real-time playback and offline export --
e.g., skipping UI updates or using higher-quality processing during export.

Dispatch/mechanics:
  KillStateHandler::isCurrentlyExporting() -- checks if AudioExportThread has a
  registered thread ID

Source:
  ScriptingApi.h:1860  isCurrentlyExporting() const
    -> KillStateHandler::isCurrentlyExporting()
