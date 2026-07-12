## getNonExistentSibling

**Examples:**

```javascript:unique-filename-export
// Title: Generate a unique filename for audio export
// Context: Exporting audio files without overwriting existing exports

const var exportDir = FileSystem.getFolder(FileSystem.Desktop);

inline function exportToUniqueFile(audioData, baseName)
{
    local targetFile = exportDir.getChildFile(baseName);

    // Get a unique name: "mix.wav" -> "mix (2).wav" -> "mix (3).wav" etc.
    targetFile = targetFile.getNonExistentSibling();

    targetFile.writeAudioFile(audioData, Engine.getSampleRate(), 24);
    Console.print("Exported to: " + targetFile.toString(3));

    return targetFile;
}

// Export a mono buffer - filename auto-increments if "render.wav" exists
const var buf = Buffer.create(44100);
exportToUniqueFile([buf], "render.wav");
```
```json:testMetadata:unique-filename-export
{
  "testable": false,
  "skipReason": "Writes audio files to the Desktop filesystem"
}
```

**Cross References:**
- `File.writeAudioFile`
