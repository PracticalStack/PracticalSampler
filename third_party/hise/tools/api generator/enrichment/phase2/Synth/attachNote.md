## attachNote

**Examples:**

```javascript:auto-release-octave-layer
// Title: Auto-release artificial octave layer when the original key is released
// Context: A synth generates an artificial note one octave above each incoming note.
// attachNote links the artificial to the original so that releasing the key
// automatically stops both, eliminating manual note-off tracking in onNoteOff.

// Required: enable the attached note buffer before using attachNote
Synth.setFixNoteOnAfterNoteOff(true);

inline function onNoteOn()
{
    local originalId = Message.getEventId();

    // Generate an artificial note one octave up
    local octaveId = Synth.addNoteOn(1, Message.getNoteNumber() + 12, Message.getVelocity(), 0);

    // Link: when the original note-off arrives, the octave note is auto-killed
    Synth.attachNote(originalId, octaveId);
}

// No onNoteOff handling needed -- the attached note is released automatically
```
```json:testMetadata:auto-release-octave-layer
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events for Message.getEventId() and Message.getNoteNumber()"
}
```

Without `attachNote`, you would need to store `octaveId` in a per-note array (indexed by note number or event ID) and manually issue `Synth.noteOffByEventId(octaveId)` in `onNoteOff`. The `attachNote` approach is cleaner when the artificial note's lifetime should exactly match the original's.
