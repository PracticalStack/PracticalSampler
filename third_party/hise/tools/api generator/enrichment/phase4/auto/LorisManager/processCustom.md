Processes the analysed partial list using a custom callback function. The callback is invoked once for every breakpoint of every partial, receiving an object with the breakpoint's properties. Modify the mutable properties directly on the object - changes are written back to the partial list after each call.

The callback object contains the following properties:

| Property | Type | Access | Description |
|----------|------|--------|-------------|
| `channelIndex` | int | read-only | Channel in the audio file |
| `partialIndex` | int | read-only | Index of the partial |
| `sampleRate` | double | read-only | Sample rate of the file |
| `rootFrequency` | double | read-only | Root frequency passed to `analyse()` |
| `time` | double | read-write | Time of the breakpoint |
| `frequency` | double | read-write | Frequency in Hz |
| `phase` | double | read-write | Phase in radians |
| `gain` | double | read-write | Amplitude |
| `bandwidth` | double | read-write | Noisiness (0.0 = pure sine, 1.0 = full noise) |

> [!Warning:Read-only properties are silently ignored] Setting `channelIndex`, `partialIndex`, `sampleRate`, or `rootFrequency` on the callback object has no effect. Only the five read-write properties are written back to the partial list.
