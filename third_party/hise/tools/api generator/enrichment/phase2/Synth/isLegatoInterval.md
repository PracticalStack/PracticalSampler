## isLegatoInterval

**Examples:**

```javascript:portamento-legato-only
// Title: Portamento triggered only on legato transitions
// Context: A synth with optional portamento uses isLegatoInterval() in the
// onNoteOn callback to detect when a new note arrives while another is held.
// The pitch glide is only applied during legato transitions, not on isolated
// note presses.

reg lastNote = -1;
reg lastEventId = -1;

inline function onNoteOn()
{
    Message.ignoreEvent(true);

    local newId = Synth.addNoteOn(1, Message.getNoteNumber(), Message.getVelocity(), 0);

    if (Synth.isLegatoInterval() && lastNote != -1)
    {
        // Legato transition: kill old note and glide
        Synth.noteOffByEventId(lastEventId);

        local delta = Message.getNoteNumber() - lastNote;
        Synth.addPitchFade(newId, 0, -delta, 0);
        Synth.addPitchFade(newId, 200, 0, 0);
    }

    lastEventId = newId;
    lastNote = Message.getNoteNumber();
}
```
```json:testMetadata:portamento-legato-only
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events for Message methods and Synth.isLegatoInterval()"
}
```

```javascript:legato-with-sustain-check
// Title: Combined legato and sustain pedal check for wind instruments
// Context: A wind instrument sampler applies legato transitions only when
// keys overlap AND the sustain pedal is NOT down. When the pedal is held,
// a different articulation (sustain retrigger) is used instead.

reg articulationType = "";

inline function onNoteOn()
{
    if (Synth.isLegatoInterval() && !Synth.isSustainPedalDown())
    {
        // Legato transition without sustain pedal - apply glide
        articulationType = "legato";
    }
    else
    {
        articulationType = "normal";
    }
}
```
```json:testMetadata:legato-with-sustain-check
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events and sustain pedal state"
}
```

**Pitfalls:**
- `isLegatoInterval()` returns `true` when zero keys are pressed (not just during overlapping notes). The internal check is `numPressedKeys != 1`, not `numPressedKeys > 1`. In `onNoteOff` after the last key is released, `isLegatoInterval()` returns `true` because the count is 0. Always combine with an explicit `lastNote != -1` or `getNumPressedKeys() > 0` check when using it in note-off callbacks.
