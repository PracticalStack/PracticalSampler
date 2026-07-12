Synchronises this player's transport to the master clock (host transport or internal clock). When enabled, playback starts and stops with the clock rather than through manual `play()` and `stop()` calls.

> [!Warning:Enable grid before syncing] The master clock grid must be enabled before calling this method - otherwise a script error is thrown. Once synced, `play()` and `stop()` become no-ops and return false. Use the TransportHandler to control transport instead. If you call `record()` while synced and stopped, recording is deferred until the clock next starts.
