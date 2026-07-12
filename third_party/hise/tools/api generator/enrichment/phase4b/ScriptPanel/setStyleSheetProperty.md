# setStyleSheetProperty | UNSAFE

Sets a CSS variable on this component that can be queried from a stylesheet. The `type` parameter determines how the value is converted to a CSS-compatible string representation.

```
setStyleSheetProperty(String variableId, var value, String type)
```

## Type Conversion Values

| type | Conversion |
|------|-----------|
| `"path"` | Path object to base64-encoded string |
| `"color"` | Integer colour to CSS `#AARRGGBB` string |
| `"%"` | Number to percentage string (0.5 becomes `"50%"`) |
| `"px"` | Number to pixel value string (10 becomes `"10px"`) |
| `"em"` | Number to em value string |
| `"vh"` | Number to viewport-height string |
| `"deg"` | Number to degree string |
| `""` | No conversion - stores value as-is |

## Pair With

- `setStyleSheetClass()` - assign a CSS class to match against stylesheet rules
- `setStyleSheetPseudoState()` - set pseudo-state selectors for conditional styling

## Source

`ScriptingApiContent.h` line ~1734
