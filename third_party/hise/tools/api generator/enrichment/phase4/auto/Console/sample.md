Records a labeled snapshot of data into the active sampling session. The data is cloned at capture time, so subsequent mutations do not affect the recorded value. Requires `startSampling()` to be called first.

> [!Warning:Requires active sampling session] Without an active session, `sample()` logs a warning and skips recording. The warning is only shown once per Console instance - subsequent calls without a session are silently skipped.
