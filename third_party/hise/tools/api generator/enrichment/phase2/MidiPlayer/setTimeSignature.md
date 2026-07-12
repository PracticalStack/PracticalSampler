## setTimeSignature

**Examples:**

```javascript:adjust-sequence-bar-count
// Title: Adjusting sequence length after writing note data
// Context: After flushing new note data to a sequence, update the time
// signature to match the desired bar count. Read-modify-write pattern
// preserves existing time signature values.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.create(4, 4, 4);

// Get the current time signature object
var ts = mp.getTimeSignature();

// Change only the bar count
ts.NumBars = 2;
mp.setTimeSignature(ts);
```
```json:testMetadata:adjust-sequence-bar-count
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getTimeSignature().NumBars", "value": 2},
    {"type": "REPL", "expression": "mp.getTimeSignature().Nominator", "value": 4}
  ]
}
```

**Pitfalls:**
- The `Tempo` property in the time signature object is read-only. Setting it has no effect - tempo is derived from the host/master clock, not the sequence metadata.
