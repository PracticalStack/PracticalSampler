Adds a pixelated noise texture overlay to the current rendering. The parameter can be a simple float (0.0-1.0) controlling the noise opacity, or a JSON object for fine-grained control:

| Property | Type | Description |
|----------|------|-------------|
| `alpha` | double | Noise opacity (0.0-1.0) |
| `monochromatic` | bool | `true` for grayscale noise, `false` for colour noise |
| `scaleFactor` | float | Scale factor for the noise texture (0.125-2.0) |
| `area` | `[x, y, w, h]` | Target area for the noise (defaults to component bounds) |

Noise images are cached internally for performance, but this caching increases memory usage proportional to the area size.

> [!Warning:Use JSON form with area in LAF callbacks] When using the simple float form inside a `ScriptLookAndFeel` callback, the area defaults to an empty rectangle because there is no parent ScriptPanel. Use the JSON object form with an explicit `area` property in LAF callbacks: `g.addNoise({"alpha": 0.05, "monochromatic": true, "area": obj.area})`.
