## toReferenceString

**Examples:**

```javascript:audio-pool-reference
// Title: Convert a user-imported audio file to a pool reference
// Context: After a user selects an audio file, convert it to a pool reference
//          string for use with AudioSampleProcessor.setFile()

const var audioDir = FileSystem.getFolder(FileSystem.AudioFiles);

// User selects a file from the AudioFiles directory
var selectedFile = audioDir.getChildFile("kick.wav");

// Convert to pool reference format: {PROJECT_FOLDER}kick.wav
var poolRef = selectedFile.toReferenceString("AudioFiles");
Console.print(poolRef); // {PROJECT_FOLDER}kick.wav

// The pool reference can be used with audio player modules
const var player = Synth.getAudioSampleProcessor("AudioPlayer1");
player.setFile(poolRef);
```
```json:testMetadata:audio-pool-reference
{
  "testable": false,
  "skipReason": "Requires an AudioSampleProcessor module named AudioPlayer1 and audio file on disk"
}
```

**Pitfalls:**
- The file must actually reside inside the specified project subdirectory for the reference string to be meaningful. The method constructs the reference string regardless of actual file location - it does not validate containment.
