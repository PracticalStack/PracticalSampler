## setSyncToMasterClock

**Examples:**

```javascript:master-clock-synced-playback
// Title: Setting up master-clock-synced MIDI playback
// Context: Configure the TransportHandler grid first, then sync all
// MidiPlayer instances. Transport is controlled via the clock, not play()/stop().

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var transportHandler = Engine.createTransportHandler();

// Enable the grid and set sync mode BEFORE syncing players
transportHandler.setEnableGrid(true, 8);
transportHandler.setSyncMode(transportHandler.PreferInternal);

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.create(4, 4, 1);
mp.setSyncToMasterClock(true);

// Transport is now driven by the clock, not play()/stop()
// Use transportHandler.startInternalClock(0) to begin playback
// Use transportHandler.stopInternalClock(0) to stop

// Verify play() is a no-op when synced
var playResult = mp.play(0);
```
```json:testMetadata:master-clock-synced-playback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "playResult", "value": 0}
  ]
}
```

**Pitfalls:**
- The master clock grid must be enabled via `TransportHandler.setEnableGrid(true, gridIndex)` before calling `setSyncToMasterClock(true)`. Otherwise a script error is thrown.
- Once synced, `play()` and `stop()` return `false` without doing anything. This is by design - use the TransportHandler to control transport.
- `record()` has special behavior when synced and stopped: it sets a flag to start recording when the clock next starts, rather than starting immediately.
