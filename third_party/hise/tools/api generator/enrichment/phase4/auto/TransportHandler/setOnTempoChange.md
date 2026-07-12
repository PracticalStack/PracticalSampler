Registers a callback that fires when the host tempo (BPM) changes. The `sync` parameter controls dispatch mode: `SyncNotification` executes on the audio thread (requires `inline function`), `AsyncNotification` dispatches to the UI thread. The callback receives the new tempo as a double. Fires immediately upon registration with the current tempo.

Registering a sync callback clears any async callback using the same function reference (and vice versa).

![Sync vs Async Callback Dispatch](timing_sync-async-dispatch.svg)
