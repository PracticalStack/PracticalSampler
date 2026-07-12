Sets a Loris analysis option to a new value. Options affect subsequent `analyse()` calls. Both numeric values (e.g. `2.0` for `windowwidth`) and string values (e.g. `"samples"` for `timedomain`) are accepted - the value is converted to a string internally. An unrecognised option ID produces an error in the console.

| Option | Default | Description |
|--------|---------|-------------|
| `"timedomain"` | `"seconds"` | Time axis domain: `"seconds"`, `"samples"`, or `"0to1"` |
| `"enablecache"` | `true` | Cache analysed partials for reuse |
| `"windowwidth"` | `1.0` | Window width scale factor (clamped to 0.125 - 4.0) |
| `"freqfloor"` | `40.0` | Lowest frequency considered harmonic content (Hz) |
| `"ampfloor"` | `90.0` | Lowest amplitude above noise floor (dB) |
| `"sidelobes"` | `90.0` | Side lobe gain of analysis window (dB) |
| `"freqdrift"` | `50.0` | Maximum frequency drift tolerance (cents) |
| `"hoptime"` | `0.0129` | Time between analysis windows (seconds) |
| `"croptime"` | `0.0129` | Crop time parameter (seconds) |
| `"bwregionwidth"` | `1.0` | Bandwidth region width |
