# Download -- Class Analysis

## Brief
Handle for active file downloads with progress, pause/resume, and abort control.

## Purpose
Download represents an active or completed HTTP file download managed by the Server class. It exposes a read-only state machine with lifecycle control methods (stop, resume, abort) and progress inspection (bytes downloaded, speed, percentage). Download objects are created exclusively by `Server.downloadFile()` and execute on the Server's background WebThread, with API methods setting atomic flags that the WebThread acts upon. The callback function receives the Download object as `this`, providing access to the `data` property object and all getter methods.

## Details

### State Machine

Downloads progress through a flag-based state machine using five atomic booleans. API methods called from the scripting thread only set flags -- the Server's WebThread reads these flags and performs the actual HTTP operations. See `getStatusText()` for the full value descriptions and `stop()`, `resume()`, `abort()` for the lifecycle control methods.

| State | getStatusText() | Description |
|-------|----------------|-------------|
| Waiting | `"Waiting"` | Created but not yet picked up by WebThread |
| Downloading | `"Downloading"` | Actively transferring data |
| Paused | `"Paused"` | Stopped via `stop()`; can be resumed |
| Completed | `"Completed"` | Transfer finished (check `data.success`) |
| Aborted | `"Aborted"` | Cancelled via `abort()`; target file deleted |

### The `data` Property Object

The `data` constant is a mutable DynamicObject accessible as `download.data` (or `this.data` inside the callback). It carries download state updated by the WebThread:

| Property | Type | Description |
|----------|------|-------------|
| `numTotal` | int | Total download size in bytes (includes pre-existing bytes on resume) |
| `numDownloaded` | int | Bytes downloaded so far (includes pre-existing bytes on resume) |
| `finished` | bool | Whether the download has completed (success or failure) |
| `success` | bool | Whether the download completed successfully |
| `aborted` | bool | Whether the download was aborted |

### Callback Model

The callback passed to `Server.downloadFile()` takes zero arguments. The Download object is bound as `this`, so all state is accessed via `this.data`, `this.getProgress()`, `this.getStatusText()`, etc. The callback fires:

- On initial start (success or connection failure)
- During transfer (throttled to every 100ms)
- On stop, abort, or completion
- On resume when the file is already fully downloaded

### Resume Mechanism

When a download is stopped and later resumed, the class checks for an existing partial file and uses HTTP Range headers to download only the remaining bytes into a temporary sibling file. On completion, the temporary file is appended to the original target. All public-facing byte counts (see `getProgress()`, `getDownloadSize()`, `getNumBytesDownloaded()`) include the pre-existing bytes, so progress reflects the total transfer.

### Download Deduplication

If `Server.downloadFile()` is called with a URL matching an already-pending download, the existing Download object's callback is replaced and the existing object is returned. Downloads are compared by URL equality.

### Concurrency Throttling

The Server enforces a maximum number of concurrent downloads (default: 1, configurable via `Server.setNumAllowedDownloads()`). Excess downloads remain in `"Waiting"` state until a slot becomes available.

## obtainedVia
`Server.downloadFile(url, params, targetFile, callback)` -- returns a Download handle.

## minimalObjectToken
dl

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| data | (DynamicObject) | JSON | Mutable object carrying download state (numTotal, numDownloaded, finished, success, aborted) | State |

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| -- | -- | -- |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Server.downloadFile(url, {}, f, function(dl){ dl.getProgress(); });` | `Server.downloadFile(url, {}, f, function(){ this.getProgress(); });` | The callback takes zero arguments. The Download object is the `this` context, not a parameter. |
| Calling `resume()` after `abort()` | Use `Server.downloadFile()` to start a new download | `abort()` deletes the target file and marks the download as finished. It cannot be resumed. |

## codeExample
```javascript:download-progress-monitor
// Title: Start a download and monitor progress
const var dl = Server.downloadFile("https://example.com/file.zip", {},
    FileSystem.getFolder(FileSystem.Downloads).getChildFile("file.zip"),
    function()
    {
        if (this.data.finished)
        {
            if (this.data.success)
                Console.print("Download complete: " + this.getDownloadedTarget());
            else
                Console.print("Download failed");
        }
        else
        {
            Console.print("Progress: " + Math.round(this.getProgress() * 100) + "%");
            Console.print("Speed: " + this.getDownloadSpeed() + " bytes/s");
        }
    });
```
```json:testMetadata:download-progress-monitor
{
  "testable": false,
  "skipReason": "Requires active HTTP connection and remote server to initiate a real download"
}
```

## Alternatives
Server -- Download objects are created by Server.downloadFile(); Server manages the HTTP connection while Download exposes per-transfer state.

## Related Preprocessors
None.

## Diagrams

### download-lifecycle
- **Brief:** Download State Machine
- **Type:** topology
- **Description:** State flow: Waiting -> Downloading -> Completed/Paused/Aborted. Transitions: WebThread picks up (Waiting->Downloading), stop() sets flag (Downloading->Paused), resume() sets flag (Paused->Waiting->Downloading), abort() sets flag (Downloading->Aborted, file deleted), JUCE finished callback (Downloading->Completed). The data.success property distinguishes successful completion from connection failure.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods are zero-parameter getters or simple flag-setters with no preconditions that could silently fail. The callback model is well-defined with no timeline dependencies.
