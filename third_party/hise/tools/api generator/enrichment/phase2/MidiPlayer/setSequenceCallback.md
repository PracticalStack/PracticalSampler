## setSequenceCallback

**Examples:**

```javascript:sequence-change-ui-refresh
// Title: Sequence change callback for UI refresh
// Context: Register a callback that fires when the MIDI data changes
// (load, edit, clear). Use this to rebuild UI elements like step
// sequencer displays. The callback fires once immediately on registration.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");

reg callbackCount = 0;

inline function onSequenceChange(player)
{
    callbackCount++;

    // Rebuild the step sequencer UI from the new MIDI data
    if (!player.isEmpty())
    {
        var ts = player.getTimeSignature();
        Console.print("Sequence changed: " + ts.NumBars + " bars, " +
                       ts.Nominator + "/" + ts.Denominator);
    }
}

mp.setSequenceCallback(onSequenceChange);
// The callback fires immediately, initializing the UI state
```
```json:testMetadata:sequence-change-ui-refresh
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "callbackCount >= 1", "value": true}
  ]
}
```

The callback receives the MidiPlayer itself as the argument, not the sequence data. Use `getEventList()`, `getTimeSignature()`, or `getNoteRectangleList()` inside the callback to access the new data.
