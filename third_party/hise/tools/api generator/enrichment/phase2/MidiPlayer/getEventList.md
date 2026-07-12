## getEventList

**Examples:**

```javascript:filter-events-by-note-number
// Title: Reading and filtering events from a MIDI sequence
// Context: Extract events, filter by type and note number, and compute
// step positions from tick timestamps for a step sequencer UI

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
const var __mp = Synth.getMidiPlayer("MidiPlayer1");
__mp.setUseTimestampInTicks(true);
__mp.create(4, 4, 1);
var setupNotes = [];
var onA = Engine.createMessageHolder();
var offA = Engine.createMessageHolder();
onA.setType(onA.NoteOn);
offA.setType(onA.NoteOff);
onA.setNoteNumber(64);
offA.setNoteNumber(64);
onA.setVelocity(100);
onA.setChannel(1);
offA.setChannel(1);
onA.setTimestamp(0);
offA.setTimestamp(240);
setupNotes.push(onA);
setupNotes.push(offA);
__mp.flushMessageList(setupNotes);
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
var events = mp.getEventList();
var tpq = mp.getTicksPerQuarter(); // 960

// Filter to only note-on events for a specific note
var noteOns = events.filter(function(e)
{
    return e.isNoteOn() && e.getNoteNumber() == 64;
});

// Convert tick timestamps to step positions (16th note grid)
var stepSize = tpq / 4; // 240 ticks per 16th note

for (e in noteOns)
{
    var stepIndex = parseInt(e.getTimestamp() / stepSize);
    var velocity = e.getVelocity() / 127.0;
    Console.print("Step " + stepIndex + ": velocity " + velocity);
}
```
```json:testMetadata:filter-events-by-note-number
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "noteOns.length", "value": 1},
    {"type": "REPL", "expression": "noteOns[0].getNoteNumber()", "value": 64}
  ]
}
```

```javascript:read-modify-write-transpose
// Title: Read-modify-write pattern for transposing notes
// Context: Get the event list, modify events in place, then flush back.
// This is the standard edit workflow for MIDI data.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
const var __mp = Synth.getMidiPlayer("MidiPlayer1");
__mp.setUseTimestampInTicks(true);
__mp.create(4, 4, 1);
var setupNotes = [];
var onB = Engine.createMessageHolder();
var offB = Engine.createMessageHolder();
onB.setType(onB.NoteOn);
offB.setType(onB.NoteOff);
onB.setNoteNumber(60);
offB.setNoteNumber(60);
onB.setVelocity(100);
onB.setChannel(1);
offB.setChannel(1);
onB.setTimestamp(0);
offB.setTimestamp(960);
setupNotes.push(onB);
setupNotes.push(offB);
__mp.flushMessageList(setupNotes);
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
var events = mp.getEventList();

for (e in events)
{
    if (e.isNoteOn() || e.isNoteOff())
        e.setNoteNumber(e.getNoteNumber() + 12);
}

mp.flushMessageList(events);
```
```json:testMetadata:read-modify-write-transpose
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getEventList()[0].getNoteNumber()", "value": 72}
  ]
}
```
