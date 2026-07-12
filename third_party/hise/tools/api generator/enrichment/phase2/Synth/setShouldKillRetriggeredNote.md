## setShouldKillRetriggeredNote

**Examples:**

```javascript:unison-voice-stacking
// Title: Disable retrigger kill for unison voice stacking
// Context: A synth with a unison/detune feature generates multiple artificial
// notes per key press, all on the same pitch. Without disabling the retrigger
// kill, each subsequent artificial note would kill the previous one, leaving
// only a single voice instead of the intended unison spread.

// Must be called in onInit before any note generation occurs
Synth.setShouldKillRetriggeredNote(false);

const var NUM_UNISON_VOICES = 4;

inline function onNoteOn()
{
    Message.ignoreEvent(true);

    // Generate N artificial voices on different channels
    for (i = 0; i < NUM_UNISON_VOICES; i++)
    {
        Synth.addNoteOn(i + 1, Message.getNoteNumber(), Message.getVelocity(), 0);
    }
}
```
```json:testMetadata:unison-voice-stacking
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events for Message methods and addNoteOn"
}
```

This is a prerequisite for any script that intentionally plays multiple overlapping voices on the same pitch - unison engines, chord generators, or layered round-robin triggers. Leave it at the default `true` for monophonic or standard polyphonic instruments where same-pitch retrigger should kill the old voice.
