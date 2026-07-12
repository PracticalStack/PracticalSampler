Registers a callback that fires on each musical beat. The callback receives the beat index (integer) and a boolean indicating whether this is the first beat of a new bar. The beat callback accounts for the time signature denominator - in 6/8 time it fires twice as often as in 3/4.

> [!Warning:First callback at next beat boundary] Does not fire immediately upon registration (unlike `setOnTempoChange` and `setOnTransportChange`). The first callback arrives at the next beat boundary.
