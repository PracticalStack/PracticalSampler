Enables or disables the high-precision grid timer at a specific musical tempo division. The `tempoFactor` is an index into the TempoSyncer note value table (0-18, mapping to note values from 1/1 whole note down to 1/64T triplet). This is a global setting - enabling the grid affects all TransportHandler instances. The grid must be enabled before `setOnGridChange` callbacks will fire.

| tempoFactor | Note Value |
|-------------|------------|
| 0 | 1/1 (Whole note) |
| 5 | 1/4 (Quarter note) |
| 8 | 1/8 (Eighth note) |
| 11 | 1/16 (Sixteenth note) |
| 14 | 1/32 (32nd note) |
| 17 | 1/64 (64th note) |
| ... | (includes dotted and triplet variants 1-18) |

When MidiPlayers are synced to the master clock via `setSyncToMasterClock(true)`, they follow the grid enabled by `setEnableGrid`. This is how a multi-channel sequencer synchronises independent channels to a single clock source.
