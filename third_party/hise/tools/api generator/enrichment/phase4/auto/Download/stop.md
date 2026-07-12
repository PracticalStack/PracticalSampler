Pauses the download, preserving the target file with whatever data has been downloaded so far. Returns `true` if the download was actively running, `false` if it was already stopped or finished. After stopping, `getStatusText()` returns `"Paused"` and the download can be continued with `resume()`. Unlike `abort()`, this does not delete the target file.

[See: Download State Machine](#diagram-download-lifecycle)

> [!Warning:Stop is asynchronous] The stop is asynchronous. When `stop()` returns, the download has not yet paused - `isRunning()` may still return `true` until the background thread processes the request.