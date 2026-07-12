## setEnableGrid

**Examples:**

```javascript:enable-grid-for-drum
// Title: Enable grid for drum sequencer with MidiPlayer sync
// Context: A drum machine enables a 1/8 note grid (tempo factor 8) and syncs
// its MidiPlayers to the master clock. The grid provides the timing backbone --
// MidiPlayers follow it automatically via setSyncToMasterClock.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.MidiProcessors.MidiPlayer, "MIDIPlayer 1", 0, builder.ChainIndexes.Midi);
builder.create(builder.MidiProcessors.MidiPlayer, "MIDIPlayer 2", 0, builder.ChainIndexes.Midi);
builder.create(builder.MidiProcessors.MidiPlayer, "MIDIPlayer 3", 0, builder.ChainIndexes.Midi);
builder.flush();

const var th0 = Engine.createTransportHandler();
th0.setSyncMode(th0.InternalOnly);
th0.stopInternalClock(0);
// --- end setup ---
const var th = Engine.createTransportHandler();

var players = [
    Synth.getMidiPlayer("MIDIPlayer 1"),
    Synth.getMidiPlayer("MIDIPlayer 2"),
    Synth.getMidiPlayer("MIDIPlayer 3")
];

// Create empty sequences so sync has something to work with
for (mp in players)
    mp.create(4, 4, 1);

// Enable 1/8 note grid for the sequencer
th.setEnableGrid(true, 8);
// Use internal clock so the example runs without a DAW
th.setSyncMode(th.PreferInternal);
// Automatically stop the internal clock when the DAW transport stops
th.stopInternalClockOnExternalStop(true);

// Sync all MidiPlayers to the master clock grid
for (mp in players)
    mp.setSyncToMasterClock(true);

// --- test-only ---
th.startInternalClock(0);
// --- end test-only ---
```
```json:testMetadata:enable-grid-for-drum
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "th.isPlaying()", "value": true},
    {"type": "REPL", "delay": 500, "expression": "Synth.getMidiPlayer('MIDIPlayer 1').getPlaybackPosition() > 0.0", "value": true}
  ]
}
```


**Cross References:**
- `MidiPlayer.setSyncToMasterClock` -- When MidiPlayers are synced to the master clock, they follow the grid enabled by `setEnableGrid`. This is how a multi-channel sequencer synchronizes independent channels to a single clock source.
