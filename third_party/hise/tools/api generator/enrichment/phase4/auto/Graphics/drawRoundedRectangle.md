Draws a rounded rectangle outline within the specified area using the current colour. The `cornerData` parameter accepts a number for uniform corner radius, or a JSON object for per-corner control:

| Property | Type | Description |
|----------|------|-------------|
| `CornerSize` | float | Radius for rounded corners in pixels |
| `Rounded` | Array | `[topLeft, topRight, bottomLeft, bottomRight]` booleans controlling which corners are rounded |

When all four `Rounded` values are `false`, a plain rectangle is drawn instead. To draw a filled rounded rectangle, use `fillRoundedRectangle`.
