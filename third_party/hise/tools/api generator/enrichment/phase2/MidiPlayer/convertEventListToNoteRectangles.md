## convertEventListToNoteRectangles

**Examples:**

```javascript:preview-edited-notes
// Title: Preview edited notes before flushing to the sequence
// Context: Unlike getNoteRectangleList() which reads from the stored sequence,
// convertEventListToNoteRectangles() operates on an arbitrary event list.
// This is useful for visualizing a merged or modified set of events.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseTimestampInTicks(true);
mp.create(4, 4, 1);

// Create a simple event list with one note
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
on.setTimestamp(0);
off.setTimestamp(960);
noteList.push(on);
noteList.push(off);

// Convert the list to rectangles for visualization
var bounds = [0, 0, 500, 200];
var noteRects = mp.convertEventListToNoteRectangles(noteList, bounds);
Console.print("Rectangles: " + noteRects.length);
```
```json:testMetadata:preview-edited-notes
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "noteRects.length", "value": 1}
  ]
}
```

Use `getNoteRectangleList()` when you want to draw the current sequence as-is. Use `convertEventListToNoteRectangles()` when you need to visualize a modified or merged event list before committing it.
