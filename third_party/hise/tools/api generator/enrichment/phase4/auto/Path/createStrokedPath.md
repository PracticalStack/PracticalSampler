Returns a new Path representing the outlined (stroked) version of the current path. The original path is not modified. The `strokeData` parameter accepts either a simple numeric thickness or a JSON object with detailed stroke properties:

| Property | Type | Values |
|----------|------|--------|
| `Thickness` | Number | Stroke width |
| `EndCapStyle` | String | `"butt"`, `"square"`, `"rounded"` |
| `JointStyle` | String | `"mitered"`, `"curved"`, `"beveled"` |

The `dotData` parameter controls dashing: pass `[]` for a solid stroke, or an array of alternating dash and gap lengths (e.g. `[10, 5]`) for a dashed pattern.

This is essential when you need to apply gradient fills, shadows, or other fill effects to arc strokes - `Graphics.drawPath` can only stroke with a solid colour, while `Graphics.fillPath` on the stroked result supports the full range of fill modes.

> [!Warning:Pass empty array for solid strokes] Always pass `[]` (empty array) for solid strokes, not an empty string. While empty strings may work silently, `[]` is the correct format.
