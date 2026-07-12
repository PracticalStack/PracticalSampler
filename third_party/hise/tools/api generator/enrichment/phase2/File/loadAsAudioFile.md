## loadAsAudioFile

**Examples:**

```javascript:preview-audio-with-gain
// Title: Preview an audio file with correct sample rate and gain
// Context: Loading a sample for audition playback through the engine

const var previewFile = FileSystem.getFolder(FileSystem.AudioFiles).getChildFile("preview.wav");

if (previewFile.isFile())
{
    var sampleRate = previewFile.loadAudioMetadata().SampleRate;
    var audioBuffer = previewFile.loadAsAudioFile();

    // Normalise mono to array format for consistent handling
    if (!Array.isArray(audioBuffer))
        audioBuffer = [audioBuffer];

    // Apply gain before playback
    var gainFactor = Engine.getGainFactorForDecibels(-6.0);

    for (ch in audioBuffer)
        ch *= gainFactor;

    Engine.playBuffer(audioBuffer, false, sampleRate);
}
```
```json:testMetadata:preview-audio-with-gain
{
  "testable": false,
  "skipReason": "Requires an audio file on disk at AudioFiles/preview.wav"
}
```

**Pitfalls:**
- Always load the metadata separately via `loadAudioMetadata()` to get the file's native sample rate. Passing the wrong sample rate to `Engine.playBuffer` will pitch-shift the audio.
- The mono-vs-array return type is a common source of bugs. Wrap the result with `if (!Array.isArray(audio)) audio = [audio];` for consistent downstream handling.
