Sets the internal clock speed for the parent synth's MIDI clock generation. The clock determines the musical subdivision at which internal timing events are produced, affecting transport-synchronised features like arpeggiators and sequencers.

| Value | Division |
|-------|----------|
| 0 | Inactive (disable clock) |
| 1 | Bar (whole note) |
| 2 | Half note |
| 4 | Quarter note |
| 8 | Eighth note |
| 16 | Sixteenth note |
| 32 | Thirty-second note |

Any value not in this table produces a script error.
