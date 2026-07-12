## setUseTimestampInTicks

**Examples:**

```javascript:tick-mode-musical-editing
// Title: Setting up tick mode for musical editing
// Context: Always enable tick mode early in initialization when building
// a step sequencer. Tick timestamps align to musical grid positions
// regardless of tempo changes.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");

// Enable tick mode before any getEventList/flushMessageList calls
mp.setUseTimestampInTicks(true);
mp.create(4, 4, 1);

// Now timestamps are in MIDI ticks (960 per quarter note)
var tpq = mp.getTicksPerQuarter(); // Always 960

// Common grid divisions in ticks:
// Whole note    = tpq * 4 = 3840
// Half note     = tpq * 2 = 1920
// Quarter note  = tpq     = 960
// 8th note      = tpq / 2 = 480
// 16th note     = tpq / 4 = 240
// 32nd note     = tpq / 8 = 120
// Triplet 8th   = tpq / 3 = 320

// Create a note at the second 16th note position
var noteList = [];
var on = Engine.createMessageHolder();
var off = Engine.createMessageHolder();
on.setType(on.NoteOn);
off.setType(on.NoteOff);
on.setNoteNumber(64);
off.setNoteNumber(64);
on.setVelocity(100);
on.setChannel(1);
off.setChannel(1);
on.setTimestamp(240); // second 16th note
off.setTimestamp(480);
noteList.push(on);
noteList.push(off);
mp.flushMessageList(noteList);

var events = mp.getEventList();

for (e in events)
{
    if (e.isNoteOn())
    {
        // Timestamp is now in ticks, e.g. 240 = second 16th note
        var stepIndex = parseInt(e.getTimestamp() / (tpq / 4));
        Console.print("Step " + stepIndex);
    }
}
```
```json:testMetadata:tick-mode-musical-editing
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tpq", "value": 960},
    {"type": "REPL", "expression": "events[0].getTimestamp()", "value": 240}
  ]
}
```

**Pitfalls:**
- This setting affects both reading (`getEventList()`) and writing (`flushMessageList()`). If you read events in tick mode, you must also flush in tick mode, or the timestamps will be misinterpreted.
