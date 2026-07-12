## addPitchFade

**Examples:**

```javascript:monophonic-glide
// Title: Monophonic glide between notes using the two-phase pitch fade pattern
// Context: A monophonic synth replaces each incoming note with an artificial one.
// When a legato transition occurs, the new note starts with an instant pitch offset
// equal to the interval between old and new notes, then fades to zero over the glide time.

reg lastNote = -1;
reg lastEventId = -1;

const var glideTimeMs = 200;

inline function onNoteOn()
{
    Message.ignoreEvent(true);

    local newId = Synth.addNoteOn(1, Message.getNoteNumber(), Message.getVelocity(), 0);

    if (lastNote != -1)
    {
        Synth.noteOffByEventId(lastEventId);

        // Phase 1: Instantly offset pitch by the interval (in semitones)
        local delta = Message.getNoteNumber() - lastNote;
        Synth.addPitchFade(newId, 0, -delta, 0);

        // Phase 2: Glide from the offset back to the note's natural pitch
        Synth.addPitchFade(newId, glideTimeMs, 0, 0);
    }

    lastEventId = newId;
    lastNote = Message.getNoteNumber();
}

inline function onNoteOff()
{
    if (Message.getNoteNumber() == lastNote)
    {
        Synth.noteOffByEventId(lastEventId);
        lastNote = -1;
    }
}
```
```json:testMetadata:monophonic-glide
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events for Message methods and addPitchFade"
}
```

The two-phase pattern works because `addPitchFade` sets the target pitch as an absolute offset from the note's original pitch, not relative to the current glide position. Phase 1 (time = 0) jumps instantly, Phase 2 fades from wherever the pitch currently is to the target.

```javascript:legato-volume-pitch-crossfade
// Title: Legato with volume crossfade between old and new notes
// Context: For smoother legato transitions, the old note fades out while
// the new note fades in, both with coordinated pitch glides.

reg eventId = -1;
reg lastNote = -1;

const var FADE_TIME = 150;
const var BEND_TIME = 200;

inline function onNoteOn()
{
    Message.ignoreEvent(true);

    if (Synth.isLegatoInterval() && eventId != -1)
    {
        local interval = Message.getNoteNumber() - lastNote;
        local fadeCoarse = -interval;

        // Fade out the old note with pitch bend toward the new pitch
        Synth.addVolumeFade(eventId, FADE_TIME, -100);
        Synth.addPitchFade(eventId, BEND_TIME, interval, 0);

        // Start the new note quietly with pitch offset, then fade in
        eventId = Synth.addNoteOn(1, Message.getNoteNumber(), Message.getVelocity(), 0);
        Synth.addVolumeFade(eventId, 0, -99);
        Synth.addVolumeFade(eventId, FADE_TIME, 0);
        Synth.addPitchFade(eventId, 0, fadeCoarse, 0);
        Synth.addPitchFade(eventId, BEND_TIME, 0, 0);
    }
    else
    {
        eventId = Synth.addNoteOn(1, Message.getNoteNumber(), Message.getVelocity(), 0);
    }

    lastNote = Message.getNoteNumber();
}
```
```json:testMetadata:legato-volume-pitch-crossfade
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events for Message methods, isLegatoInterval, and addPitchFade"
}
```
