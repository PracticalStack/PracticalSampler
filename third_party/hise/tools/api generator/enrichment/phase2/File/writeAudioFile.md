## writeAudioFile

**Examples:**

```javascript:export-stereo-unique-name
// Title: Export a stereo audio buffer to WAV with unique filename
// Context: Exporting rendered audio without overwriting existing files

const var exportDir = FileSystem.getFolder(FileSystem.Desktop);

inline function exportAudio(bufferData, fileName)
{
    local targetFile = exportDir.getChildFile(fileName);

    // Avoid overwriting - get a unique sibling name like "mix (2).wav"
    targetFile = targetFile.getNonExistentSibling();

    // Trim trailing silence before writing
    local channelsToWrite = [
        bufferData[0].trim(0, 0),
        bufferData[1].trim(0, 0)
    ];

    targetFile.writeAudioFile(channelsToWrite, Engine.getSampleRate(), 24);
    Console.print("Exported: " + targetFile.toString(0));

    return targetFile;
}

// Usage: export a stereo sine wave
const var left = Buffer.create(44100);
const var right = Buffer.create(44100);

for (i = 0; i < 44100; i++)
{
    left[i] = Math.sin(2.0 * Math.PI * 440.0 * i / 44100.0);
    right[i] = left[i];
}

exportAudio([left, right], "mix.wav");
```
```json:testMetadata:export-stereo-unique-name
{
  "testable": false,
  "skipReason": "Writes audio files to the Desktop filesystem"
}
```

**Pitfalls:**
- The file extension determines the output format. Use `.wav` for lossless WAV, `.flac` for compressed lossless, `.ogg` for lossy. Using an unsupported extension like `.mp3` triggers a script error.
- Use `getNonExistentSibling()` before writing when generating export files to avoid silently overwriting user files. The original file is deleted before the write begins, so if the write fails, the original is lost.

**Cross References:**
- `File.getNonExistentSibling`
