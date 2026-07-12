Draws the outline (stroke) of a Path object using the current colour. If `area` is an `[x, y, w, h]` array, the path is scaled to fit the rectangle. If `area` is omitted or not an array, the path is drawn at its original coordinates.

The `strokeStyle` parameter accepts two formats:

1. **Number** - simple stroke thickness (e.g., `2.0`)
2. **JSON object** - advanced stroke configuration:

| Property | Type | Description |
|----------|------|-------------|
| `Thickness` | float | Stroke width in pixels |
| `EndCapStyle` | String | `"butt"`, `"square"`, or `"rounded"` |
| `JointStyle` | String | `"mitered"`, `"curved"`, or `"beveled"` |
