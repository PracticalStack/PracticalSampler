Configures the modulation and UI ranges for a MatrixModulator. Pass a JSON object with `InputRange` (the UI knob range) and `OutputRange` (the scaled modulation output range) sub-objects:

**InputRange properties:**

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `min` | Number | 0.0 | Minimum knob value |
| `max` | Number | 1.0 | Maximum knob value |
| `middlePosition` | Number | 0.5 | Centre position of the knob |
| `mode` | String | -- | Text converter mode (e.g., `"NormalizedPercentage"`). Also used as the display mode when a text converter is supplied |
| `stepSize` | Number | 0.0 | Step size for discrete values (0.0 for continuous) |

**OutputRange properties:**

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `min` | Number | 0.0 | Minimum modulation output |
| `max` | Number | 1.0 | Maximum modulation output |
| `middlePosition` | Number | 0.5 | Centre position of the output range |
| `stepSize` | Number | 0.0 | Step size for discrete values (0.0 for continuous) |
| `UseMidPositionAsZero` | Boolean | false | When true, the mid position is treated as zero for the output signal |

When supplying a text converter, only the `mode` property in `InputRange` is used for the lookup - other properties do not affect the converter.

> [!Warning:Only works on MatrixModulator instances] Only works on MatrixModulator instances. Calling this on any other modulator type silently does nothing.
