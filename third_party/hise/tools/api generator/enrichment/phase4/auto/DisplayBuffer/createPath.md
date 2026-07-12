Creates a Path object from the ring buffer data, scaled to fit the given destination rectangle. The visual result adapts automatically to the buffer source type (oscilloscope, FFT, envelope). The returned path is open - call `Path.closeSubPath()` before using `Graphics.fillPath()` to create a filled waveform display.

The `sourceRange` parameter encodes both value and sample ranges as `[minValue, maxValue, startSample, endSample]`:

| Element | Purpose | Typical values |
|---|---|---|
| `sourceRange[0]` | Minimum value | `-1.0` (bipolar) or `0.0` (unipolar) |
| `sourceRange[1]` | Maximum value | `1.0` |
| `sourceRange[2]` | Start sample index | `0` |
| `sourceRange[3]` | End sample index | `-1` for the full buffer |

The `normalisedStartValue` sets the vertical starting position of the path relative to the source range. Use `0.0` for waveforms and unipolar meters (path starts at the range minimum), or `1.0` to start at the range maximum (for inverted plotters).

> [!Warning:Non-standard sourceRange packing] The `sourceRange` array is not a spatial rectangle. The first two elements define the value normalisation range and the last two define the sample range. Swapping value and sample indices produces incorrect scaling.
