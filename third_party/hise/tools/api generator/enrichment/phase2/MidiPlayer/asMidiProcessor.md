## asMidiProcessor

**Examples:**

```javascript:read-playback-speed-attribute
// Title: Reading playback speed via MidiProcessor attributes
// Context: Access the MidiPlayer's module attributes (like PlaybackSpeed)
// through the MidiProcessor interface for controlling loop speed

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
const var proc = mp.asMidiProcessor();

// MidiPlayer attribute indices:
// 0 = CurrentPosition, 1 = CurrentSequence, 2 = CurrentTrack,
// 3 = LoopEnabled, 4 = LoopStart, 5 = LoopEnd, 6 = PlaybackSpeed

var speed = proc.getAttribute(6);
Console.print("Playback speed: " + speed); // e.g. 1.0 = normal

// Use speed to calculate effective loop duration
var effectiveSpeed = speed * 8; // Convert to step grid units
```
```json:testMetadata:read-playback-speed-attribute
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "proc.getAttribute(6)", "value": 1.0}
  ]
}
```

The MidiProcessor reference provides access to `getAttribute()` / `setAttribute()` for the underlying module parameters. This is the only way to read or set properties like PlaybackSpeed, LoopStart, and LoopEnd from script.
