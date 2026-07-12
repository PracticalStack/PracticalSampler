## setUseGlobalUndoManager

**Examples:**

```javascript:enable-undo-for-sequencer
// Title: Enabling undo for a multi-channel sequencer
// Context: Enable the global undo manager during initialization so that
// all subsequent MIDI edits (flushMessageList, setTimeSignature) can be
// undone with Engine.undo()

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseGlobalUndoManager(true);
mp.setUseTimestampInTicks(true);
mp.create(4, 4, 1);

// Create and flush some test data
var noteList = [];
var on = Engine.createMessageHolder();
var off = Engine.createMessageHolder();
on.setType(on.NoteOn);
off.setType(on.NoteOff);
on.setNoteNumber(60);
off.setNoteNumber(60);
on.setVelocity(100);
on.setChannel(1);
off.setChannel(1);
on.setTimestamp(0);
off.setTimestamp(960);
noteList.push(on);
noteList.push(off);
mp.flushMessageList(noteList);

// Undo reverts the edit
Engine.undo();
```
```json:testMetadata:enable-undo-for-sequencer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.isSequenceEmpty(1)", "value": 1}
  ]
}
```

**Pitfalls:**
- Undo is disabled by default. Calling `undo()` or `redo()` without first calling `setUseGlobalUndoManager(true)` throws a script error.
- The undo manager is global (shared via `Engine.undo()`), so undo operations from any MidiPlayer share the same undo stack.
