## addNoteOn

**Examples:**

```javascript:unison-detune-stacking
// Title: Unison voice stacking - generate multiple detuned voices per incoming note
// Context: A synthesizer with a unison/detune feature replaces each incoming note with
// N artificial notes on different MIDI channels. Each channel feeds a different detune
// modulator. The event IDs are tracked per note number for proper cleanup.

// In onInit:
Synth.setShouldKillRetriggeredNote(false);

const var NUM_VOICES = 4;

// Pre-allocate a 2D array: one event ID list per MIDI note
const var eventIds = [];
eventIds.reserve(128);
for(i = 0; i < 128; i++)
{
    eventIds[i] = [];
    eventIds[i].reserve(NUM_VOICES);
}

// In onNoteOn:
inline function onNoteOn()
{
    Message.ignoreEvent(true);

    local noteEventIds = eventIds[Message.getNoteNumber()];
    noteEventIds.clear();

    for(i = 0; i < NUM_VOICES; i++)
    {
        // Each voice uses a different MIDI channel for separate detune routing
        noteEventIds[i] = Synth.addNoteOn(i + 1, Message.getNoteNumber(), Message.getVelocity(), 0);
    }
}

// In onNoteOff:
inline function onNoteOff()
{
    Message.ignoreEvent(true);

    local noteEventIds = eventIds[Message.getNoteNumber()];

    for (id in noteEventIds)
        Synth.noteOffByEventId(id);

    noteEventIds.clear();
}
```
```json:testMetadata:unison-detune-stacking
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events for Message methods and addNoteOn/noteOffByEventId"
}
```

**Pitfalls:**
- When generating multiple artificial voices per key, you must call `Synth.setShouldKillRetriggeredNote(false)` in `onInit`. Otherwise the synth kills the previous voice on the same pitch as soon as the next artificial note arrives, and you end up with only one voice instead of N.
