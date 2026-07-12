Sets the MIDI event type using one of the type constants exposed on the MessageHolder instance (e.g. `mh.NoteOn`, `mh.Controller`). A newly created MessageHolder has type Empty, which must be changed to a valid type before it can be used with `Synth.addMessageFromHolder()`.

| Type | Value | Semantically relevant fields |
|---|---|---|
| `NoteOn` | 1 | note number, velocity, event ID |
| `NoteOff` | 2 | note number, event ID |
| `Controller` | 3 | controller number, controller value |
| `PitchBend` | 4 | 14-bit pitch wheel value |
| `Aftertouch` | 5 | note number (poly) or pressure (mono) |
| `ProgramChange` | 13 | number field |