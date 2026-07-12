## callOnBackgroundThread

**Examples:**

```javascript:file-scanning-progress
// Title: File scanning with progress and thread-safe UI updates
// Context: A shared BackgroundTask scans a directory tree on a background thread,
// reporting progress via the loading overlay. The .lock(Threads.Scripting) block
// ensures thread-safe access to scripting APIs from the background thread.

const var fileScanner = Engine.createBackgroundTask("FileScanner");
fileScanner.setForwardStatusToLoadingThread(true);
fileScanner.setTimeOut(4000);

inline function scanFiles(task)
{
    local root = FileSystem.getFolder(FileSystem.Samples);
    local allFiles = FileSystem.findFiles(root, "*.wav", true);

    for (i = 0; i < allFiles.length; i++)
    {
        if (task.shouldAbort())
            return;

        task.setProgress(i / allFiles.length);
        task.setStatusMessage("Scanning: " + allFiles[i].toString(1));

        {
            .lock(Threads.Scripting);

            // Thread-safe operations that touch the scripting engine
            task.setProperty("lastFile", allFiles[i].toString(1));
            task.setProperty("fileCount", i + 1);
        }
    }
};

// Multiple UI actions can trigger the same task -- the previous scan
// is automatically stopped before the new one starts.
fileScanner.callOnBackgroundThread(scanFiles);
```
```json:testMetadata:file-scanning-progress
{
  "testable": false,
  "skipReason": "Requires file system access and background thread execution"
}
```

**Pitfalls:**
- When reusing a single BackgroundTask across multiple triggers, be aware that calling `callOnBackgroundThread()` while a previous task is running blocks the calling thread for up to `timeout` ms. For I/O-heavy tasks, call `sendAbortSignal(true)` explicitly before starting the new task to make the cancellation intent clear and avoid unexpected UI freezes.
