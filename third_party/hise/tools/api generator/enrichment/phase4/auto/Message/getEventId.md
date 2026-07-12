Returns the unique event ID assigned to the current event. Note-on and its matching note-off share the same event ID, which is what enables per-voice operations like `Synth.addVolumeFade()` and `Synth.addPitchFade()` to target a specific sounding voice.

> [!Warning:IDs wrap around at 65536] Event IDs are unsigned 16-bit integers that wrap around at 65536. Do not assume that older notes have lower IDs than newer ones.
