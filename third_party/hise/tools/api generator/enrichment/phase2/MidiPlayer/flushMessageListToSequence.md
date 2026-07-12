## flushMessageListToSequence

**Examples:**

```javascript:write-pattern-to-specific-index
// Title: Writing note data to a specific pattern bank
// Context: In a multi-pattern sequencer, write programmatically generated
// notes to a specific pattern index. This avoids switching the active
// sequence just to write data.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseTimestampInTicks(true);
mp.clearAllSequences();
mp.create(4, 4, 1);
mp.create(4, 4, 1);

const var STEP_SIZE = mp.getTicksPerQuarter() / 4; // 16th note grid

inline function writePatternData(patternIndex, noteNumber, velocityArray)
{
    local notes = [];
    notes.reserve(velocityArray.length * 2);

    for (i = 0; i < velocityArray.length; i++)
    {
        if (velocityArray[i] == 0.0)
            continue;

        local on = Engine.createMessageHolder();
        local off = Engine.createMessageHolder();

        on.setType(on.NoteOn);
        off.setType(on.NoteOff);
        on.setChannel(1);
        off.setChannel(1);
        on.setNoteNumber(noteNumber);
        off.setNoteNumber(noteNumber);
        on.setVelocity(parseInt(velocityArray[i] * 127));
        on.setTimestamp(i * STEP_SIZE);
        off.setTimestamp(i * STEP_SIZE + STEP_SIZE - 1);

        notes.push(on);
        notes.push(off);
    }

    // Write to one-based pattern index without changing the active sequence
    mp.flushMessageListToSequence(notes, patternIndex + 1);
}

// Write to pattern 2 (internal index 1 -> one-based = 2)
writePatternData(1, 60, [1.0, 0.0, 0.5, 0.0]);
```
```json:testMetadata:write-pattern-to-specific-index
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getEventListFromSequence(2).length", "value": 4}
  ]
}
```
