## flushMessageList

**Examples:**

```javascript:build-step-sequence-from-velocities
// Title: Programmatically building a step sequence from slider data
// Context: Convert a velocity array (from a SliderPack or UI data) into
// MIDI note-on/note-off pairs and flush them into the current sequence

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseTimestampInTicks(true);
mp.create(4, 4, 1);

const var NOTE_NUMBER = 64;
const var NUM_STEPS = 16;
const var STEP_SIZE = mp.getTicksPerQuarter() / 4; // 16th note = 240 ticks

// Example velocity values (0.0 = silent, >0 = active)
const var velocities = [1.0, 0.0, 0.5, 0.0, 1.0, 0.0, 0.75, 0.0,
                        1.0, 0.0, 0.5, 0.0, 1.0, 0.0, 0.75, 0.0];

var noteList = [];
noteList.reserve(NUM_STEPS * 2);

for (i = 0; i < NUM_STEPS; i++)
{
    if (velocities[i] > 0.0)
    {
        var on = Engine.createMessageHolder();
        var off = Engine.createMessageHolder();

        on.setType(on.NoteOn);
        off.setType(on.NoteOff);

        on.setChannel(1);
        off.setChannel(1);

        on.setNoteNumber(NOTE_NUMBER);
        off.setNoteNumber(NOTE_NUMBER);

        on.setVelocity(parseInt(velocities[i] * 127));

        on.setTimestamp(i * STEP_SIZE);
        off.setTimestamp(i * STEP_SIZE + STEP_SIZE - 1);

        noteList.push(on);
        noteList.push(off);
    }
}

mp.flushMessageList(noteList);
```
```json:testMetadata:build-step-sequence-from-velocities
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getEventList().length", "value": 16}
  ]
}
```

**Pitfalls:**
- Every note-on must have a matching note-off in the list. Unpaired note-ons will sustain indefinitely during playback.
- Always call `setUseTimestampInTicks(true)` before flushing when the timestamps are in musical ticks, or the engine will interpret them as sample counts.
