Casts this handle to a MidiPlayer reference for accessing MIDI file playback, editing, and visualisation methods. The cast is the inverse of `MidiPlayer.asMidiProcessor()` - use it when you have a generic MidiProcessor handle and need playback control.

> [!Warning:Throws error if not a MidiPlayer] Throws a script error if the underlying module is not a MidiPlayer. Only use this on modules you know to be MidiPlayer instances.
