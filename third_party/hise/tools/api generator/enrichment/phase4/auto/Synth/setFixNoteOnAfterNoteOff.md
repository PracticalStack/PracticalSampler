Enables a safety mechanism that prevents stuck notes when a note-off is processed before its corresponding note-on - a timing edge case that arises with delayed or artificial events. Call this once in `onInit` on the script processor that performs the MIDI manipulation.

When enabled, the system adds two automatic checks:

1. When a note-off is about to be processed, the event queue is scanned for a future note-on with the same event ID. If found, the pending note-on is cancelled.
2. When any API method creates an artificial note-off, the same queue scan is performed.

The first check catches stuck notes from `Message.delayEvent()` and the second covers artificial note pairs created with `Synth.addNoteOn` / `Synth.addNoteOff`. This method is also required before using `Synth.attachNote()` - calling `attachNote` without it throws an error.

> [!Warning:Per-sound-generator setting only] This is a per-sound-generator setting. Calling it in your Interface script does not affect child sound generators that perform their own MIDI manipulation. Enable it on each script processor that needs it.
