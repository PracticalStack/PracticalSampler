## startTimer

**Examples:**

```javascript:arpeggiator-tempo-synced
// Title: Arpeggiator with tempo-synced timer intervals
// Context: A step sequencer or arpeggiator calculates the timer interval
// from the host tempo and uses startTimer to drive note generation.
// The timer fires on the audio thread for sample-accurate timing.

reg currentStep = 0;
const var NUM_STEPS = 8;

inline function onController()
{
    // Start arpeggiator when a note is held (simplified)
    if (Message.getControllerNumber() == 64 && Message.getControllerValue() > 0)
    {
        // Calculate interval from host BPM for eighth notes
        local bpm = Engine.getHostBpm();
        local eighthNoteSeconds = 30.0 / bpm;
        Synth.startTimer(eighthNoteSeconds);
    }
}

inline function onTimer()
{
    local note = 48 + currentStep;
    local id = Synth.playNote(note, 100);

    // Auto-release after half the step duration
    Synth.noteOffDelayedByEventId(id, parseInt(Engine.getSampleRate() * Synth.getTimerInterval() * 0.5));

    currentStep = (currentStep + 1) % NUM_STEPS;
}
```
```json:testMetadata:arpeggiator-tempo-synced
{
  "testable": false,
  "skipReason": "Requires MIDI callback context for onController/onTimer and active synth for playNote"
}
```

```javascript:delayed-sustain-debounce
// Title: Delayed sustain release with timer-based debounce
// Context: A piano plugin keeps a background resonance note alive while keys
// are held. When the last key is released, a 1-second timer starts. If no
// new key arrives before the timer fires, the background note is released.
// This prevents abrupt cutoffs during normal playing.

reg backgroundNoteId = -1;

inline function onNoteOff()
{
    if (Synth.getNumPressedKeys() == 0)
        Synth.startTimer(1.0);
}

inline function onNoteOn()
{
    Synth.stopTimer();

    if (backgroundNoteId == -1)
        backgroundNoteId = Synth.playNote(0, 127);
}

inline function onTimer()
{
    if (backgroundNoteId != -1)
    {
        Synth.noteOffByEventId(backgroundNoteId);
        backgroundNoteId = -1;
    }

    Synth.stopTimer();
}
```
```json:testMetadata:delayed-sustain-debounce
{
  "testable": false,
  "skipReason": "Requires MIDI callback context for onNoteOn/onNoteOff/onTimer and active synth for playNote"
}
```

Calling `stopTimer()` from inside `onTimer` is the standard pattern for one-shot timer behavior. In the debounce pattern above, `startTimer` restarts the countdown each time it's called - if a new key arrives within the window, `stopTimer` cancels the pending release.
