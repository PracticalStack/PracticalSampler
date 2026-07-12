## create

**Examples:**

```javascript:create-pattern-banks
// Title: Creating multiple pattern bank sequences
// Context: Initialize a player with multiple empty sequences (pattern banks)
// for a step sequencer. Each sequence is a separate pattern slot.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
const var NUM_PATTERNS = 4;

mp.setUseTimestampInTicks(true);

// Clear any existing data
mp.clearAllSequences();

// Create 4 pattern banks, each 2 bars of 4/4
for (i = 0; i < NUM_PATTERNS; i++)
    mp.create(4, 4, 2);

// Select the first pattern (one-based indexing)
mp.setSequence(1);
```
```json:testMetadata:create-pattern-banks
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getNumSequences()", "value": 4},
    {"type": "REPL", "expression": "mp.getTimeSignature().NumBars", "value": 2}
  ]
}
```

`create()` appends a new sequence to the player's sequence list. Call `clearAllSequences()` first to avoid accumulating sequences across preset loads or reinitializations.
