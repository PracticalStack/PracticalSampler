## fromReferenceString

**Examples:**

```javascript:resolve-audio-reference-for-display
// Title: Resolve an AudioSampleProcessor reference to a File for display
// Context: AudioSampleProcessor.getFilename() returns a HISE reference string
// like "{PROJECT_FOLDER}sound.wav". To show the filename in the UI or navigate
// to the parent directory, resolve it to a File object first.

const var player = Synth.getAudioSampleProcessor("AudioPlayer1");

inline function getCurrentAudioFile()
{
    local refString = player.getFilename();

    if (!refString.length)
        return undefined;

    // Resolve the {PROJECT_FOLDER} reference to an actual File
    return FileSystem.fromReferenceString(refString, FileSystem.AudioFiles);
}

inline function getDisplayName()
{
    local f = getCurrentAudioFile();

    if (isDefined(f))
        return f.toString(1); // Filename without extension

    return "No file loaded";
}

// Navigate to sibling files in the same directory
inline function getNextFile(delta)
{
    local currentFile = getCurrentAudioFile();

    if (!isDefined(currentFile))
        return;

    local parentDir = currentFile.getParentDirectory();
    local siblings = FileSystem.findFiles(parentDir, "*.wav,*.aif", false);

    for (i = 0; i < siblings.length; i++)
    {
        if (siblings[i].isSameFileAs(currentFile))
        {
            local newIndex = Math.range(i + delta, 0, siblings.length - 1);
            player.setFile(siblings[newIndex].toString(0));
            return;
        }
    }
}
```
```json:testMetadata:resolve-audio-reference-for-display
{
  "testable": false,
  "skipReason": "Requires Synth.getAudioSampleProcessor('AudioPlayer1') which needs an AudioLooper module in the signal chain. Depends on loaded audio files."
}
```

**Pitfalls:**
- In exported plugins, references that resolve to embedded resources (compiled into the binary) cause `fromReferenceString()` to return `undefined`. Code that works in the HISE IDE - where files exist on disk - will fail silently in the compiled plugin. Use `isDefined()` on the result, and consider whether your use case actually needs a disk file or just needs the reference string for display.
