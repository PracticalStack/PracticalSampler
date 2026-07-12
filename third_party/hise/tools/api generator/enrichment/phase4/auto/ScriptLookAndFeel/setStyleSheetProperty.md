Sets a CSS variable that can be read in stylesheets via `var(--variableId)`. This connects HISEScript logic to CSS rendering - script code can update visual properties dynamically without rewriting the stylesheet. Calling this method automatically repaints all components using this LAF.

The third argument specifies a type converter that transforms the HISEScript value into a CSS-compatible format:

| Type | Expected Value | Description |
| --- | --- | --- |
| `""` | any string | No conversion; passes the raw string to CSS |
| `"px"` | a number | Pixel value (e.g. `25` becomes `25px`) |
| `"%"` | a float 0.0-1.0 | Percentage value (e.g. `0.8` becomes `80%`) |
| `"color"` | a colour value | Converts HISEScript colours (e.g. `Colours.red` or `0xFF00FF00`) to CSS format (`#FF00FF00`) |
| `"path"` | a Path object | Converts the path to a base64 string for use as `background-image` |
| `"class"` | a string | Writes one or more CSS class selectors to the component |

HISE automatically injects changes to the four standard colour properties (`bgColour`, `itemColour`, `itemColour2`, `textColour`) as CSS variables without needing this method. Reference them directly in CSS:

```
button { background-color: var(--bgColour); }
```

When the same variable is set on both the LAF (via this method) and on an individual component (via `component.setStyleSheetProperty()`), the component-level value always takes precedence, regardless of call order.

To inspect the resolved value of each variable for a specific component, right-click the component in the Interface Designer and choose **Show CSS debugger**. This displays all current variable values, the component's CSS selector hierarchy, and any inherited stylesheets.
