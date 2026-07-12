Sets the callback that fires when a background task starts and finishes. The callback receives two boolean arguments: `isFinished` and `wasCancelled`. It is called with `(false, false)` at task start, `(true, false)` on normal completion, and `(true, true)` on abort. The BackgroundTask is available as `this` inside the callback.

> [!Warning:Callback may run concurrently with task] The finish callback runs on the scripting thread and may execute concurrently with the still-running background task function. Avoid accessing shared state without the thread-safe property store.

> [!Warning:Only triggered by callOnBackgroundThread] Not triggered by `killVoicesAndCall()` - only by `callOnBackgroundThread()` and `runProcess()`.
