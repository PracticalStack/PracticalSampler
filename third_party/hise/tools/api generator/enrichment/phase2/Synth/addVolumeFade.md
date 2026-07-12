## addVolumeFade

**Examples:**

```javascript:fade-to-kill-loop
// Title: Fade-to-kill pattern for stopping a looped note on restart
// Context: A loop processor stops the previous loop iteration by fading it out
// over 100ms before the next iteration starts. The -100 target volume triggers
// the auto-kill behavior: HISE generates both the volume fade and an automatic
// note-off one sample after the fade completes.

reg lastId = -1;

inline function killLastNote()
{
    if (lastId != -1)
    {
        // -100 = fade to silence AND auto-release the voice
        Synth.addVolumeFade(lastId, 100, -100);
        lastId = -1;
    }
}
```
```json:testMetadata:fade-to-kill-loop
{
  "testable": false,
  "skipReason": "Requires MIDI callback context with an active artificial note to apply the volume fade"
}
```

The `-100` target volume is a special sentinel value. Any other negative dB value (e.g., `-99`, `-60`) performs a normal volume fade without killing the voice. Only exactly `-100` triggers the combined fade + auto-note-off behavior.

```javascript:volume-crossfade-legato
// Title: Volume crossfade between old and new legato notes
// Context: For smooth legato transitions, the old note fades out while the
// new note fades in from silence. Both notes overlap briefly during the crossfade.

reg eventId = -1;

const var CROSSFADE_MS = 150;

inline function onNoteOn()
{
    Message.ignoreEvent(true);

    if (eventId != -1)
    {
        // Fade out old note and auto-kill it
        Synth.addVolumeFade(eventId, CROSSFADE_MS, -100);
    }

    // Start new note at silence, then fade in
    eventId = Synth.addNoteOn(1, Message.getNoteNumber(), Message.getVelocity(), 0);
    Synth.addVolumeFade(eventId, 0, -99);
    Synth.addVolumeFade(eventId, CROSSFADE_MS, 0);
}
```
```json:testMetadata:volume-crossfade-legato
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events for Message methods and addNoteOn"
}
```

Note that `addVolumeFade(id, 0, -99)` sets the initial volume to -99 dB (effectively silent) without killing the voice, and the subsequent `addVolumeFade(id, CROSSFADE_MS, 0)` fades up to 0 dB over the crossfade time.
