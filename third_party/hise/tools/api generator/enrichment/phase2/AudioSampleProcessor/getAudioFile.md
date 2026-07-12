## getAudioFile

**Examples:**

```javascript:wavetable-audio-file-bridge
// Title: Bridging to the AudioFile API for wavetable drag-and-drop
// Context: A wavetable synth exposes its audio file slot as a drop target
// so users can drag audio files onto the UI to load new wavetables.

const var wt1 = Synth.getAudioSampleProcessor("WavetableSynth1");
const var wtAudioFile = wt1.getAudioFile(0);

// The AudioFile reference can be used for file drop targets,
// broadcaster attachment, or direct sample data access
wtAudioFile.loadFile("{PROJECT_FOLDER}default_wavetable.wav");
```
```json:testMetadata:wavetable-audio-file-bridge
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module and an audio file in the project pool."
}
```

```javascript:broadcaster-file-change-monitor
// Title: Monitoring file changes across multiple processors with a Broadcaster
// Context: A missing-file watcher attaches a broadcaster to the AudioFile.Content
// of multiple AudioSampleProcessor modules to detect when files are loaded or cleared.

const var NUM_PLAYERS = 4;
const var playerIds = [];

for (i = 0; i < NUM_PLAYERS; i++)
    playerIds.push("Player" + (i + 1));

const var fileWatcher = Engine.createBroadcaster({
    "id": "fileWatcher",
    "args": ["processor", "index", "file"]
});

// Attach to audio file content changes on all players
fileWatcher.attachToComplexData("AudioFile.Content", playerIds, 0, "file change monitor");

fileWatcher.addListener("", "log file changes", function(processor, index, file)
{
    Console.print(processor + " file changed");
});
```
```json:testMetadata:broadcaster-file-change-monitor
{
  "testable": false,
  "skipReason": "Requires multiple AudioSampleProcessor modules (Player1-4) in the module tree."
}
```

**Cross References:**
- `Broadcaster.attachToComplexData` -- the primary way to react to file changes across multiple AudioSampleProcessor modules.
