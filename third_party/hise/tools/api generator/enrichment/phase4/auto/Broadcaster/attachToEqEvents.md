Registers the broadcaster as a source that fires when EQ-related events occur on the specified module(s). The `eventTypes` parameter accepts one or more of the following strings, or an empty string to subscribe to all:

| Event Type | Description |
|---|---|
| `"BandAdded"` | A filter band was added |
| `"BandRemoved"` | A filter band was removed |
| `"BandSelected"` | A filter band was selected by user interaction |
| `"FFTEnabled"` | The FFT display was toggled |

The target module must be a parametric EQ processor (`CurveEq`). Passing a non-EQ module produces an error. For addressing individual band parameters via the standard attribute system, use the formula `attributeIndex = attributeType + bandIndex * bandOffset`.
