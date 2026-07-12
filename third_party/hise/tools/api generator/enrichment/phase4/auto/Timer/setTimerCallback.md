Registers the function to call on each timer tick. The callback receives no arguments; `this` inside the callback refers to the Timer instance, so use `this.stopTimer()` for self-stopping patterns. Calling `setTimerCallback` replaces any previously registered callback.

> [!Warning:Runs on message thread, not audio] Timer callbacks run on the message thread, not the audio thread. They are safe for UI operations but not suitable for sample-accurate timing. For beat-synced callbacks, use `TransportHandler` instead.
