## getFilename

**Examples:**

```javascript:browse-from-current-directory
// Title: Checking if a file is loaded and resolving its directory
// Context: File browsers should open in the directory of the currently loaded file.
// getFilename() returns a pool reference string - convert it to a File for path operations.

const var player = Synth.getAudioSampleProcessor("AudioLooper1");

inline function onBrowseButton(component, value)
{
    if (value)
    {
        // Default to the Samples folder
        local startDir = FileSystem.getFolder(FileSystem.Samples);

        // If a file is already loaded, start from its directory
        if (player.getFilename().length)
            startDir = FileSystem.fromReferenceString(player.getFilename(), FileSystem.AudioFiles).getParentDirectory();

        FileSystem.browse(startDir, false, "*.wav", function(newFile)
        {
            player.setFile(newFile.toString(0));
        });
    }
}
```
```json:testMetadata:browse-from-current-directory
{
  "testable": false,
  "skipReason": "Requires an AudioLooper module and FileSystem.browse user interaction."
}
```

```javascript:serialize-loaded-filenames
// Title: Serializing loaded files for custom preset save
// Context: When using a custom preset model, collect all loaded filenames
// from an array of AudioSampleProcessor handles for JSON serialization.

const var NUM_PLAYERS = 4;
const var players = [];

for (i = 0; i < NUM_PLAYERS; i++)
    players.push(Synth.getAudioSampleProcessor("Player" + (i + 1)));

inline function savePresetData()
{
    local obj = {};
    local audioFiles = [];

    for (p in players)
        audioFiles.push(p.getFilename());

    obj.AudioFiles = audioFiles;
    return obj;
}

// Invoke to show the returned structure
Console.print(trace(savePresetData()));
```
```json:testMetadata:serialize-loaded-filenames
{
  "testable": false,
  "skipReason": "Requires multiple AudioSampleProcessor modules (Player1-4) in the module tree."
}
```

**Pitfalls:**
- The returned string is a pool reference (e.g., `{PROJECT_FOLDER}loop.wav` or an absolute path), not a display-friendly name. Use `FileSystem.fromReferenceString()` to convert it to a File object for UI display or path operations.
