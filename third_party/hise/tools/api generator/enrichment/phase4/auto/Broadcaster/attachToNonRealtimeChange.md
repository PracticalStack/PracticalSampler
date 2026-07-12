Registers the broadcaster as a source that fires when the audio engine switches between realtime and non-realtime rendering modes (e.g. during a DAW bounce/export). The callback receives `true` when entering offline mode and `false` when returning to realtime.

This method automatically enables realtime mode on the broadcaster. All listeners added afterwards must be realtime-safe (inline functions in exported plugins). The callback executes synchronously on the thread that triggers the mode change, which may be the audio thread.
