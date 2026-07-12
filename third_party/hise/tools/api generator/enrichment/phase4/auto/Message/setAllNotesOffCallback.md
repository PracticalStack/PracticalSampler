Registers a callback that fires when an AllNotesOff MIDI event is received. AllNotesOff events do not trigger `onNoteOff` or `onController`, so this is the only way to respond to them - for example, to reset held-note tracking or clear internal state.

The callback takes zero parameters and must be declared as an `inline function` since it runs on the audio thread.

> [!Warning:Use inline function for safety] Using a regular `function` instead of `inline function` will be flagged as unsafe in the HISE IDE but may silently fail in exported plugins where the safety check is compiled out.
