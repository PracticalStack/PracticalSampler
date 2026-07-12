Sets a CSS variable on this component that can be referenced from a stylesheet. The `type` parameter controls how the value is converted to a CSS-compatible string.

| Type | Conversion |
|------|-----------|
| `"color"` | Integer colour to `#AARRGGBB` string |
| `"%"` | Number to percentage (0.5 becomes `"50%"`) |
| `"px"` | Number to pixel value |
| `"em"` | Number to em value |
| `"vh"` | Number to viewport-height value |
| `"deg"` | Number to degree value |
| `"path"` | Path object to base64-encoded string |
| `""` | No conversion - stored as-is |
