## setFile

**Examples:**

```javascript:load-midi-with-track-selection
// Title: Loading a MIDI file from the pool with track selection
// Context: Load a MIDI file by pool reference, clear existing sequences,
// select the new one, and set the active track

const var mp = Synth.getMidiPlayer("MidiPlayer1");

inline function loadMidiFile(fileName, trackIndex)
{
    local path = "{PROJECT_FOLDER}" + fileName + ".mid";
    mp.setFile(path, true, true);   // clear existing, select new
    mp.setTrack(trackIndex);        // select track (one-based)
}

loadMidiFile("pattern_01", 1);
```
```json:testMetadata:load-midi-with-track-selection
{
  "testable": false,
  "skipReason": "Requires MIDI files in the project pool which cannot be created programmatically"
}
```

```javascript:browse-and-load-from-pool
// Title: Browsing and loading from the embedded MIDI file pool
// Context: Get the list of available MIDI files and load one by index

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
var files = mp.getMidiFileList();

Console.print("Available MIDI files: " + files.length);

if (files.length > 0)
    mp.setFile(files[0], true, true);
```
```json:testMetadata:browse-and-load-from-pool
{
  "testable": false,
  "skipReason": "Requires MIDI files in the project pool; getMidiFileList() returns empty in a clean project"
}
```

```javascript:clear-player-sequences
// Title: Clearing a player by loading an empty filename
// Context: Pass an empty string to setFile() as a no-op, or clear with
// clearAllSequences() for a full reset

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.create(4, 4, 1);

// Clear all loaded MIDI data
mp.clearAllSequences();
```
```json:testMetadata:clear-player-sequences
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getNumSequences()", "value": 0}
  ]
}
```
