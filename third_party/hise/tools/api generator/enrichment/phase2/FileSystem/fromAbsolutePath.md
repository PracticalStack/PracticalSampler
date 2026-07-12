## fromAbsolutePath

**Examples:**

```javascript:reconstruct-file-from-stored-path
// Title: Reconstruct a File from a stored path string
// Context: When saving file paths to JSON (for caches, recent files, or
// favorites), you store the absolute path as a string. To work with
// the file again, reconstruct the File object with fromAbsolutePath().

// Saving a path for later
var recentFiles = [];

inline function addToRecentFiles(file)
{
    local path = file.toString(0); // Full absolute path as string
    recentFiles.push(path);

    // Persist to AppData
    FileSystem.getFolder(FileSystem.AppData)
              .getChildFile("recentFiles.json")
              .writeObject(recentFiles);
}

// Restoring files from stored paths
inline function loadRecentFiles()
{
    local stored = FileSystem.getFolder(FileSystem.AppData)
                             .getChildFile("recentFiles.json")
                             .loadAsObject();

    if (!isDefined(stored))
        return [];

    local validFiles = [];

    for (path in stored)
    {
        local f = FileSystem.fromAbsolutePath(path);

        // Verify the file still exists before adding
        if (isDefined(f) && f.isFile())
            validFiles.push(f);
    }

    return validFiles;
}
```
```json:testMetadata:reconstruct-file-from-stored-path
{
  "testable": false,
  "skipReason": "Depends on filesystem state (AppData JSON file, existing files) that varies between machines. Demonstrates a read-modify-write pattern that requires real files."
}
```

```javascript:file-drop-callback-conversion
// Title: Handle file drop callbacks
// Context: ScriptPanel's setFileDropCallback provides filenames as path
// strings, not File objects. Use fromAbsolutePath() to convert them.

const var dropZone = Content.getComponent("DropPanel");
const var player = Synth.getAudioSampleProcessor("AudioPlayer1");

inline function loadSample(file)
{
    player.setFile(file.toString(0));
}

dropZone.setFileDropCallback("Drop & Hover", "*.wav", function(obj)
{
    if (obj.drop)
        loadSample(FileSystem.fromAbsolutePath(obj.fileName));

    this.data.hover = obj.hover;
    this.repaint();
});
```
```json:testMetadata:file-drop-callback-conversion
{
  "testable": false,
  "skipReason": "Requires Content.getComponent('DropPanel') and Synth.getAudioSampleProcessor('AudioPlayer1') which do not exist. File drop interaction cannot be triggered programmatically."
}
```
