## saveAsMidiFile

**Examples:**

```javascript:edit-and-save-midi-workflow
// Title: MIDI file edit-and-save workflow
// Context: Load a MIDI file, process the events (remap notes, adjust
// velocities), then save back to a new location

const var mp = Synth.getMidiPlayer("MidiPlayer1");

inline function processAndSave(inputFile, outputFile, trackIndex)
{
    mp.setFile(inputFile, true, true);
    mp.setTrack(trackIndex);

    local events = mp.getEventList();

    // Remap note numbers using a conversion table
    for (e in events)
    {
        local nn = e.getNoteNumber();

        if (e.isNoteOn())
            e.setVelocity(Math.min(127, parseInt(e.getVelocity() * 1.1)));

        e.setNoteNumber(nn + 12);
    }

    mp.flushMessageList(events);
    mp.saveAsMidiFile(outputFile, trackIndex - 1); // track index is zero-based here
}
```
```json:testMetadata:edit-and-save-midi-workflow
{
  "testable": false,
  "skipReason": "Requires MIDI files in the project pool and performs file I/O"
}
```

After saving, the MIDI file pool is automatically reloaded. The saved file will appear in `getMidiFileList()` immediately.
