Forwards `setProgress()` and `setStatusMessage()` calls to HISE's built-in loading overlay - the same UI that appears during sample loading. When enabled, three integration points become active:

- `ScriptPanel.setLoadingCallback()` fires when the task starts and stops
- `Engine.getPreloadMessage()` returns the current status message
- `Engine.getPreloadProgress()` returns the current progress value

Call this before starting the task. The forwarding flag is set automatically when the background thread starts and cleared when it finishes.

> [!Warning:Does not lock actual loading thread] This captures the loading notification system but does not lock the actual loading thread. If a samplemap load or other loading-thread operation runs while your task is active, it causes glitches and inconsistent notifications.
