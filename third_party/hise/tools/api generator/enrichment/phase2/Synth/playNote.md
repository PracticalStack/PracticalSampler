## playNote

**Examples:**

```javascript:one-shot-preview-button
// Title: One-shot note preview from a UI button
// Context: A mixer strip or channel selector button triggers a short audition
// of the selected sound. playNote starts the note; noteOffDelayedByEventId
// schedules an automatic note-off after a fixed duration.

inline function previewSound(component, value)
{
    if (value)
    {
        local note = 36 + channelButtons.indexOf(component);
        local id = Synth.playNote(note, 127);

        // Auto-release after ~1 second (at 44100 Hz sample rate)
        Synth.noteOffDelayedByEventId(id, 44100);
    }
}
```
```json:testMetadata:one-shot-preview-button
{
  "testable": false,
  "skipReason": "Requires MIDI callback context with channelButtons array and active synth for playNote"
}
```

```javascript:background-sustain-note
// Title: Background sustain note controlled by pedal and key state
// Context: A piano plugin keeps a background resonance note playing while any
// keys are held or the sustain pedal is down. A timer provides a delayed release
// so the background note doesn't cut off immediately when the last key is released.

reg id = -1;
reg pedal = false;

inline function handlePedal()
{
    if (Message.getControllerNumber() == 64)
    {
        // Pedal released while no keys held: start delayed release
        if (Message.getControllerValue() < 64 && pedal && Synth.getNumPressedKeys() == 0)
            Synth.startTimer(1.0);

        pedal = Message.getControllerValue() > 64;

        if (pedal)
        {
            Synth.stopTimer();
            if (id == -1)
                id = Synth.playNote(0, 127);
        }
    }
}

inline function handleNoteOn()
{
    if (id == -1)
        id = Synth.playNote(0, 127);

    Synth.stopTimer();
}

inline function handleNoteOff()
{
    if (!pedal && Synth.getNumPressedKeys() == 0)
        Synth.startTimer(1.0);
}

inline function handleTimer()
{
    if (id != -1)
    {
        Synth.noteOffByEventId(id);
        id = -1;
    }
    Synth.stopTimer();
}
```
```json:testMetadata:background-sustain-note
{
  "testable": false,
  "skipReason": "Requires MIDI callback context for Message methods, pedal events, note events, and timer callbacks"
}
```

This pattern combines `playNote`, `noteOffByEventId`, `getNumPressedKeys`, `startTimer`, and `stopTimer` into a cohesive sustain-management system. The timer acts as a debounce - it gives the player a short window to press another key before the background note is released.
