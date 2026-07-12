## attachToComplexData

**Examples:**

```javascript:watch-audio-file-content
// Title: Watching audio file content changes across multiple player modules
// Context: A drum machine with multiple audio players needs to update UI state
// (waveform displays, sample labels, mixer icons) whenever any player loads
// a new audio file. One broadcaster watches all players at once.

const var NUM_PLAYERS = 4;

const var sampleBc = Engine.createBroadcaster({
    "id": "SampleUpdater",
    "args": ["processorId", "index", "value"]
});

// Watch audio file slot 0 across all player modules
const var playerIds = [];

for (i = 0; i < NUM_PLAYERS; i++)
    playerIds.push("Player" + (i + 1));

sampleBc.attachToComplexData(
    "AudioFile.Content",
    playerIds,
    0,
    "audioFileWatcher"
);

sampleBc.addListener("", "onSampleChange", function(processorId, index, value)
{
    Console.print(processorId + " loaded new audio file");
});
```
```json:testMetadata:watch-audio-file-content
{
  "testable": false,
  "skipReason": "Requires multiple AudioPlayer modules (Player1-Player4) that cannot be created via Builder API, and audio file content changes require loading actual audio files"
}
```

```javascript:watch-slider-pack-content
// Title: Watching slider pack pattern data for changes
// Context: A step sequencer stores pattern data in slider packs. A broadcaster
// watches the slider pack content to react when patterns are edited or loaded.

const var patternBc = Engine.createBroadcaster({
    "id": "PatternWatcher",
    "args": ["processorId", "index", "value"]
});

// Watch slider pack slots 0-3 on the Interface processor
patternBc.attachToComplexData(
    "SliderPack.Content",
    "Interface",
    [0, 1, 2, 3],
    "patternData"
);

patternBc.addListener("", "onPatternEdit", function(processorId, index, value)
{
    Console.print("Pattern " + index + " was edited");
});
```
```json:testMetadata:watch-slider-pack-content
{
  "testable": false,
  "skipReason": "SliderPack complex data slots on the Interface processor must be pre-configured in the project and content changes require user interaction or specific file operations"
}
```

Queue mode is automatically enabled when watching multiple processors or multiple data indices, ensuring no changes are lost during rapid edits.
