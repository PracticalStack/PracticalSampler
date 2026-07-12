Injects a note-on event through the virtual keyboard input pipeline, as if the user pressed a key on the on-screen keyboard. Unlike `playNote` or `addNoteOn`, this routes through the standard MIDI input path so `isKeyDown()`, `getNumPressedKeys()`, and the on-screen keyboard all reflect the note correctly. No event ID is returned - use `Synth.noteOffFromUI()` to release notes started with this method.

> [!Warning:Pair only with noteOffFromUI] Always pair with `noteOffFromUI`, never with `noteOffByEventId`. Notes from this method are real (non-artificial) events with no event ID tracking.
