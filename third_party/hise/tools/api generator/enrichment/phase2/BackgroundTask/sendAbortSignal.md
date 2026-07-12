## sendAbortSignal

**Examples:**

```javascript:abort-then-restart
// Title: Abort-then-restart pattern for shared background tasks
// Context: When multiple UI actions share a single BackgroundTask (e.g. a file
// scanner), explicitly abort the current operation before starting a new one.
// This avoids the implicit blocking that callOnBackgroundThread performs when
// a task is already running.

const var scanner = Engine.createBackgroundTask("Scanner");
scanner.setTimeOut(4000);

inline function scanFolder(task)
{
    local files = FileSystem.findFiles(
        FileSystem.getFolder(FileSystem.Samples), "*.wav", true
    );

    for (i = 0; i < files.length; i++)
    {
        if (task.shouldAbort())
            return;

        task.setProgress(i / files.length);
    }
};

inline function startScan()
{
    // Explicitly abort any running scan before starting a new one
    scanner.sendAbortSignal(true);
    scanner.callOnBackgroundThread(scanFolder);
};
```
```json:testMetadata:abort-then-restart
{
  "testable": false,
  "skipReason": "Requires background thread lifecycle"
}
```
