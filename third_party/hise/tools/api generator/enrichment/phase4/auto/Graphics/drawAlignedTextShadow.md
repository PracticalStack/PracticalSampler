Renders a blurred shadow behind text at the specified alignment. This method only draws the shadow, not the text itself. To display both, call `drawAlignedTextShadow` first, then `drawAlignedText` with the same text, area, and alignment.

The `shadowData` parameter is a JSON object with these properties:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Colour` | Colour | `0xFF000000` | Shadow colour |
| `Offset` | `[x, y]` | `[0, 0]` | Shadow offset in pixels |
| `Radius` | int | `0` | Blur radius in pixels (0 = no blur) |
| `Spread` | int | `0` | Shadow spread in pixels |
| `Inner` | bool | `false` | `true` for inner shadow, `false` for drop shadow |
