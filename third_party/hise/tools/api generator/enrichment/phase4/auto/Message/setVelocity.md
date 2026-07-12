Sets the velocity of the current note-on event. Only works in `onNoteOn` - not `onNoteOff` or other callbacks. For a level property that works across all event types and both note-on and note-off, use `Message.setGain()` instead.

> [!Warning:Zero velocity may trigger note-off] Always ensure the velocity is at least 1. A velocity of 0 is technically valid in HISE, but downstream MIDI output or DAW processing may interpret it as a note-off per the MIDI specification.
