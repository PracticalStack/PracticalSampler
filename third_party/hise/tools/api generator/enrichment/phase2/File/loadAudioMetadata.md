## loadAudioMetadata

**Examples:**

```javascript:check-metadata-before-loading
// Title: Check audio metadata before loading full sample data
// Context: Inspecting channel count and sample rate to prepare for playback

const var audioDir = FileSystem.getFolder(FileSystem.AudioFiles);
const var audioFile = audioDir.getChildFile("sample.wav");

var metadata = audioFile.loadAudioMetadata();

if (isDefined(metadata))
{
    Console.print("Format: " + metadata.Format);
    Console.print("Sample Rate: " + metadata.SampleRate);
    Console.print("Channels: " + metadata.NumChannels);
    Console.print("Duration: " + (metadata.NumSamples / metadata.SampleRate) + "s");

    // Use metadata to decide whether to load as mono or stereo
    var audio = audioFile.loadAsAudioFile();

    if (metadata.NumChannels == 1)
        audio = [audio];

    Engine.playBuffer(audio, false, metadata.SampleRate);
}
```
```json:testMetadata:check-metadata-before-loading
{
  "testable": false,
  "skipReason": "Requires an audio file on disk at AudioFiles/sample.wav"
}
```

**Pitfalls:**
- Returns `undefined` silently if the file does not exist or is not a recognized audio format. Always check with `isDefined()` before accessing properties - unlike `loadAsObject`, no script error is reported on failure.
- Use this method instead of `loadAsAudioFile` when you only need format information. It reads only the file header, not the full sample data.
