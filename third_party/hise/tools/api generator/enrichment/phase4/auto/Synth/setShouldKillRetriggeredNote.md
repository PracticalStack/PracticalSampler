Controls whether the parent synth automatically kills a playing voice when a new note-on arrives for the same pitch. When enabled (the default), retriggering a key immediately stops the previous voice. When disabled, both voices coexist, allowing note stacking on the same pitch.

Disable this in `onInit` for unison scripts or any design where multiple artificial voices are generated per key press on the same note number. Without it, each subsequent `addNoteOn` on the same pitch kills the previous one, leaving only a single voice.
