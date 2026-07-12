Injects a note-off event through the virtual keyboard input pipeline, as if the user released a key on the on-screen keyboard. Use this to stop notes started with `Synth.playNoteFromUI()`. Unlike `noteOffByEventId`, this routes through the standard MIDI input path and updates keyboard state tracking (`isKeyDown`, `getNumPressedKeys`).

> [!Warning:Pair only with playNoteFromUI] Always pair with `playNoteFromUI`, never with `playNote` or `addNoteOn`. Notes injected through the UI pipeline are real events with no event ID tracking. Mixing UI and artificial note-off methods causes stuck notes.
