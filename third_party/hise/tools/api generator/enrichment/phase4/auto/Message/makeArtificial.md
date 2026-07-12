Converts the current event into an artificial event and returns its new event ID. The event ID is required for per-voice operations like `Synth.addVolumeFade()` and `Synth.addPitchFade()` that target a specific sounding voice. This method is idempotent - calling it on an already-artificial event returns the existing ID without creating a duplicate.

For note-on events, a new sequential event ID is assigned. For note-off events, the method finds and assigns the matching note-on's event ID so the voice can be properly released.

> [!Warning:Note-off pops matching note-on tracker] On note-off events, this method pops the matching note-on from the internal event tracker. If `makeArtificial()` was never called on the corresponding note-on, no match is found and the note-off is automatically ignored, preventing voice release.
