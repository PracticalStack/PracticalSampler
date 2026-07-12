## noteOffDelayedByEventId

**Examples:**

```javascript:one-shot-note-preview
// Title: One-shot note preview with fixed-duration auto-release
// Context: A channel selector or preset browser triggers a short audition
// of a sound when the user clicks. The note is scheduled to release after
// a fixed number of samples, so no manual note-off tracking is needed.

inline function previewSound(component, value)
{
    if (value)
    {
        local note = 36 + channelButtons.indexOf(component);
        local id = Synth.playNote(note, 127);

        // Release after ~1 second at 44100 Hz
        Synth.noteOffDelayedByEventId(id, 44100);
    }
}

for (b in channelButtons)
    b.setControlCallback(previewSound);
```
```json:testMetadata:one-shot-note-preview
{
  "testable": false,
  "skipReason": "Requires MIDI callback context with channelButtons array of UI components and active synth for playNote"
}
```

This is the idiomatic pattern for fire-and-forget note previews. Unlike `noteOffByEventId` (which requires a separate note-off call, typically in a different callback), `noteOffDelayedByEventId` schedules the release immediately after the note-on. The delay is in samples, so multiply by `Engine.getSampleRate()` to convert from seconds.
