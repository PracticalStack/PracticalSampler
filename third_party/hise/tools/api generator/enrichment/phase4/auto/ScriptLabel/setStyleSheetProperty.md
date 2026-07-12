Defines a CSS variable on this component so stylesheets can read it. The `type` argument controls how the value is converted before storage.

| Value | Description |
|---|---|
| `"path"` | Converts a Path object to a base64 string. |
| `"color"` | Converts an integer colour to a `#AARRGGBB` string. |
| `"%"` | Converts a number to a percentage string, for example `0.5` to `50%`. |
| `"px"` | Converts a number to a pixel string, for example `10` to `10px`. |
| `"em"` | Converts a number to an em string. |
| `"vh"` | Converts a number to a viewport-height string. |
| `"deg"` | Converts a number to a degree string. |
| `""` | Stores the value as-is without conversion. |
