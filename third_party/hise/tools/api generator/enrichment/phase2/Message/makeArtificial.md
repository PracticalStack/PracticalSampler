## makeArtificial

**Examples:**

```javascript:monophonic-voice-management
// Title: Monophonic voice management with event ID tracking
// Context: A monophonic script needs stable event IDs to target
// specific voices for volume/pitch fades. makeArtificial() converts
// the incoming event and returns its event ID.

reg lastEventId = -1;
reg lastNote = -1;

function onNoteOn()
{
    local id = Message.makeArtificial();

    if (lastEventId != -1)
    {
        // Fade out the previous note before starting the new one
        Synth.addVolumeFade(lastEventId, 100, -100);
    }

    lastEventId = id;
    lastNote = Message.getNoteNumber();
}

function onNoteOff()
{
    if (Message.getNoteNumber() == lastNote)
    {
        lastEventId = -1;
        lastNote = -1;
    }
}
```
```json:testMetadata:monophonic-voice-management
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

```javascript:portamento-event-id-tracking
// Title: Portamento with per-note event ID tracking
// Context: A polyphonic portamento script stores the artificial event ID
// per note number so each voice can be independently pitch-faded.

reg noteIds = [];
noteIds.reserve(128);

for (i = 0; i < 128; i++)
    noteIds.push(-1);

reg lastPlayedNote = -1;

const var glideTime = Content.addKnob("GlideTime", 10, 10);
glideTime.setRange(0, 500, 1);

function onNoteOn()
{
    local note = Message.getNoteNumber();

    // makeArtificial is idempotent: if called twice on the same event,
    // the second call returns the existing ID without creating a duplicate
    noteIds[note] = Message.makeArtificial();

    if (lastPlayedNote != -1 && lastPlayedNote != note)
    {
        local interval = note - lastPlayedNote;
        Synth.addPitchFade(noteIds[note], 0, -interval, 0);
        Synth.addPitchFade(noteIds[note], parseInt(glideTime.getValue()), 0, 0);
    }

    lastPlayedNote = note;
}
```
```json:testMetadata:portamento-event-id-tracking
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

**Cross References:**
- `Message.makeArtificialOrLocal`
- `Synth.addVolumeFade`
- `Synth.addPitchFade`
