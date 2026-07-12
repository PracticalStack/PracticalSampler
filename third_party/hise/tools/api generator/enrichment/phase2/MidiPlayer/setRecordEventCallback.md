## setRecordEventCallback

**Examples:**

```javascript:quantize-and-filter-recording
// Title: Quantize-and-filter recording callback for a drum sequencer
// Context: A record event callback that filters out notes not matching the
// current channel and quantizes timestamps to the grid. This runs on the
// audio thread so it must be an inline function.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseTimestampInTicks(true);

const var CHANNEL_NOTE = 36; // The note number this channel responds to

inline function onRecordEvent(e)
{
    if (e.isNoteOn())
    {
        local number = e.getNoteNumber();

        // Filter out notes that don't belong to this channel
        if (number != CHANNEL_NOTE)
        {
            e.ignoreEvent(true);
            return;
        }

        // Quantize to 16th note grid
        local tpq = mp.getTicksPerQuarter(); // 960
        local gridSize = tpq / 4;            // 240 ticks = 16th note
        local ts = e.getTimestamp();
        local quantised = Math.round(ts / gridSize) * gridSize;
        e.setTimestamp(quantised);
    }
}

mp.setRecordEventCallback(onRecordEvent);
```
```json:testMetadata:quantize-and-filter-recording
{
  "testable": false,
  "skipReason": "Record event callback only fires during active recording with incoming MIDI events on the audio thread"
}
```

**Pitfalls:**
- The callback must reference the MidiPlayer via `this` (not a captured variable) when used with multiple MidiPlayer instances sharing the same callback function. Inside the record event callback, `this` refers to the MidiPlayer that triggered it.
