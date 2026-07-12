# Download -- Method Analysis

## abort

**Signature:** `Integer abort()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Sets two atomic boolean flags only. No allocations, no locks.
**Minimal Example:** `var ok = {obj}.abort();`

**Description:**
Aborts the download, marks it for cancellation, and deletes the target file. Internally sets the `shouldAbort` atomic flag to `true` and then calls `stop()`. The actual abort logic (file deletion, state cleanup) is executed by the Server's WebThread in `stopInternal()` when it observes the flag. Returns `true` if the download was running at the time of the call, `false` if it was already stopped or finished.

After abort, `getStatusText()` returns `"Aborted"`, `data.aborted` is `true`, `data.finished` is `true`, and `data.success` is `false`. The target file is deleted from disk. A final callback invocation occurs after the abort completes. The download cannot be resumed after abort -- calling `resume()` returns `false` because the `shouldAbort` flag blocks it.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Abort is asynchronous relative to the API call. When `abort()` returns, the file has not yet been deleted and `getStatusText()` may still return `"Downloading"`. The actual deletion happens on the WebThread's next iteration (up to 500ms later).

**Cross References:**
- `$API.Download.stop$`
- `$API.Download.resume$`
- `$API.Download.getStatusText$`

## getDownloadedTarget

**Signature:** `ScriptObject getDownloadedTarget()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new ScriptFile object (heap allocation via `new ScriptFile`).
**Minimal Example:** `var target = {obj}.getDownloadedTarget();`

**Description:**
Returns the target file as a `File` scripting object, regardless of whether the download has completed, succeeded, or even started. The method unconditionally creates a new `File` object wrapping the target path that was specified when `Server.downloadFile()` was called. It does not check the download state -- the returned `File` may reference a path that does not yet exist (download not started), is partially written (download in progress), or has been deleted (after `abort()`).

**Parameters:**

(No parameters.)

**Pitfalls:**
- The method name suggests it returns the target only after success, but it always returns the file path regardless of download state. Check `data.finished` and `data.success` before using the returned file.

**Cross References:**
- `$API.Server.downloadFile$`
- `$API.Download.abort$`

## getDownloadSize

**Signature:** `Double getDownloadSize()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads two int64 member variables and returns their sum as a double. No allocations, no locks.
**Minimal Example:** `var size = {obj}.getDownloadSize();`

**Description:**
Returns the total download size in bytes, including any pre-existing bytes from a previous partial download when resuming. The value is derived from the raw JUCE `totalLength_` field plus `existingBytesBeforeResuming`. Returns 0.0 before the download starts or if the server does not report a content length.

This method reads the internal `totalLength_` member directly (updated by the JUCE progress callback) rather than reading from the `data` DynamicObject. It is distinct from `data.numTotal` in that it does not double-count the resume offset.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Download.getNumBytesDownloaded$`
- `$API.Download.getProgress$`

## getDownloadSpeed

**Signature:** `Integer getDownloadSpeed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads atomic bool and two int64 members. No allocations, no locks.
**Minimal Example:** `var speed = {obj}.getDownloadSpeed();`

**Description:**
Returns the current download speed in bytes per second. The speed is measured using a sliding one-second window: bytes transferred in the current second are accumulated in `bytesInCurrentSecond`, and when a full second elapses, the value is moved to `bytesInLastSecond`. The method returns the maximum of both counters to avoid momentary zero readings at window boundaries. Returns 0 when the download is not actively running (`isRunning()` is false).

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Download.isRunning$`
- `$API.Download.getNumBytesDownloaded$`
- `$API.Download.getProgress$`

## getFullURL

**Signature:** `String getFullURL()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var url = {obj}.getFullURL();`

**Description:**
Returns the full URL string of this download as passed to `Server.downloadFile()`. The URL is returned without POST data (the `false` parameter to `URL::toString()`). This value is immutable for the lifetime of the Download object.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Server.downloadFile$`

## getNumBytesDownloaded

**Signature:** `Double getNumBytesDownloaded()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads two int64 member variables and returns their sum as a double. No allocations, no locks.
**Minimal Example:** `var bytes = {obj}.getNumBytesDownloaded();`

**Description:**
Returns the number of bytes downloaded so far, including any pre-existing bytes from a previous partial download when resuming. The value is derived from the raw JUCE `bytesDownloaded_` field plus `existingBytesBeforeResuming`. Returns 0.0 before the download starts.

This method reads the internal `bytesDownloaded_` member directly (updated by the JUCE progress callback) rather than reading from the `data` DynamicObject. It is distinct from `data.numDownloaded` in that it does not double-count the resume offset.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Download.getDownloadSize$`
- `$API.Download.getProgress$`

## getProgress

**Signature:** `Double getProgress()`
**Return Type:** `Double`
**Call Scope:** unsafe
**Call Scope Note:** Reads from a DynamicObject via `getProperty()` which involves string-based property lookup.
**Minimal Example:** `var pct = {obj}.getProgress();`

**Description:**
Returns the download progress as a ratio from 0.0 to 1.0. Calculated as `numDownloaded / numTotal` where both values are read from the `data` DynamicObject and then have `existingBytesBeforeResuming` added. Returns 0.0 when the total size is unknown (0 bytes).

**Parameters:**

(No parameters.)

**Pitfalls:**
- [BUG] During resumed downloads, progress is double-counted. The `data.numDownloaded` and `data.numTotal` properties already include `existingBytesBeforeResuming` (added in the `progress()` callback at line 1437-1438), but `getProgress()` adds it again at lines 1304-1305. For fresh downloads (where `existingBytesBeforeResuming` is 0) the result is correct. For resumed downloads, the numerator and denominator are both inflated by the same offset, so the ratio may still appear approximately correct, but the intermediate values are wrong. For accurate progress during resumes, use `data.numDownloaded / data.numTotal` directly.

**Cross References:**
- `$API.Download.getDownloadSize$`
- `$API.Download.getNumBytesDownloaded$`

## getStatusText

**Signature:** `String getStatusText()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var status = {obj}.getStatusText();`

**Description:**
Returns a human-readable string describing the current download state. The return value is determined by checking atomic state flags in priority order: `isRunning_` > `shouldAbort` > `isFinished` > `isWaitingForStop`.

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `"Downloading"` | Download is actively transferring data |
| `"Aborted"` | Download was cancelled via `abort()` |
| `"Completed"` | Download finished (check `data.success` to distinguish success from failure) |
| `"Paused"` | Download was stopped via `stop()`, can be resumed |
| `"Waiting"` | Download is queued but not yet started by the WebThread |

**Parameters:**

(No parameters.)

**Pitfalls:**
- `"Completed"` does not imply success. A connection failure also results in `"Completed"` status with `data.success` set to `false`. Always check `data.success` alongside the status text.

**Cross References:**
- `$API.Download.isRunning$`
- `$API.Download.stop$`
- `$API.Download.abort$`

## isRunning

**Signature:** `Integer isRunning()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads a single atomic boolean. No allocations, no locks, no string involvement.
**Minimal Example:** `var active = {obj}.isRunning();`

**Description:**
Returns `true` if the download is currently actively transferring data, `false` otherwise. The method reads the internal `isRunning_` atomic boolean flag, which is set to `true` when the Server's WebThread begins the HTTP transfer (in `start()` or `resumeInternal()`) and set back to `false` when the transfer completes (`finished()`), is stopped (`stopInternal()`), or is aborted. A download in `"Waiting"` or `"Paused"` state returns `false`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Download.getStatusText$`
- `$API.Download.stop$`
- `$API.Download.resume$`

## resume

**Signature:** `Integer resume()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Sets a single atomic boolean flag. No allocations, no locks, no string involvement.
**Minimal Example:** `var ok = {obj}.resume();`

**Description:**
Requests resumption of a stopped download. Sets the `isWaitingForStart` atomic flag to `true`, which signals the Server's WebThread to pick up the download on its next iteration (up to 500ms). The WebThread then calls `resumeInternal()`, which checks the existing target file size, sends an HTTP Range request for the remaining bytes, and downloads them to a temporary sibling file. Returns `true` if the resume request was accepted, `false` if the download is currently running, already finished, or was aborted.

The guard condition `!isRunning() && !isFinished && !shouldAbort` means resume is only possible when the download is in a stopped/paused state. Once a download completes (successfully or due to connection failure) or is aborted, it cannot be resumed -- a new download must be started via `Server.downloadFile()`.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns `false` silently when the download has already finished or was aborted. There is no error message distinguishing "cannot resume because finished" from "cannot resume because aborted". Check `getStatusText()` to determine why `resume()` returned `false`.

**Cross References:**
- `$API.Download.stop$`
- `$API.Download.abort$`
- `$API.Download.isRunning$`
- `$API.Download.getStatusText$`

**DiagramRef:** download-lifecycle

## stop

**Signature:** `Integer stop()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Sets a single atomic boolean flag. No allocations, no locks, no string involvement.
**Minimal Example:** `var ok = {obj}.stop();`

**Description:**
Requests that the download be paused. Sets the `isWaitingForStop` atomic flag to `true`, which signals the Server's WebThread to stop the active transfer on its next iteration (up to 500ms). The WebThread then calls `stopInternal()`, which nulls the JUCE `DownloadTask`, flushes any temporary resume file data to the target, sets `data.finished` to `true` and `data.success` to `false`, and fires the callback. Returns `true` if the download was actively running at the time of the call, `false` if it was not running (already stopped, waiting, finished, or aborted).

After stopping, `getStatusText()` returns `"Paused"` and the download can be resumed with `resume()`. The target file is preserved on disk with whatever data has been downloaded so far. Unlike `abort()`, stop does not delete the target file.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Stop is asynchronous relative to the API call. When `stop()` returns `true`, the download has not yet actually stopped -- the `isWaitingForStop` flag has been set but `stopInternal()` has not yet executed. `isRunning()` may still return `true` until the WebThread processes the flag (up to 500ms later).

**Cross References:**
- `$API.Download.resume$`
- `$API.Download.abort$`
- `$API.Download.isRunning$`
- `$API.Download.getStatusText$`

**DiagramRef:** download-lifecycle
