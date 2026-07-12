## setForwardStatusToLoadingThread

**Examples:**

```javascript:loading-overlay-setup
// Title: Configuring a background task with loading overlay and extended timeout
// Context: The standard setup pattern for a background task that performs I/O work:
// enable loading overlay forwarding and set an appropriate timeout before launching
// any work. This gives users visual feedback via the built-in sample loading UI.

const var task = Engine.createBackgroundTask("BatchProcessor");

// Enable the loading overlay -- setProgress() and setStatusMessage()
// will automatically update the built-in sample loading UI
task.setForwardStatusToLoadingThread(true);

// Increase timeout for I/O-heavy work (default 500ms is too short)
task.setTimeOut(2000);

inline function processFiles(t)
{
    local files = FileSystem.findFiles(
        FileSystem.getFolder(FileSystem.Samples), "*.wav", false
    );

    for (i = 0; i < files.length; i++)
    {
        if (t.shouldAbort())
            return;

        // Both calls update the loading overlay automatically
        t.setProgress(i / files.length);
        t.setStatusMessage("Processing: " + files[i].toString(1));
    }
};

task.callOnBackgroundThread(processFiles);
```
```json:testMetadata:loading-overlay-setup
{
  "testable": false,
  "skipReason": "Requires background thread and loading overlay system"
}
```
