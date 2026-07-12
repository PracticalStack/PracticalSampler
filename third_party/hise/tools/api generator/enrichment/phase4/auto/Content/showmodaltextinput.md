Shows a text input overlay attached to a component. Pass a JSON properties object to configure the editor's position, size, colours, and font, and a callback that fires when the input is dismissed. The callback receives two arguments: a status flag (`1` if the user pressed Enter, `0` if they pressed Escape or the editor lost focus) and the text string the user typed.

The text input is mutually exclusive - only one input box is visible at a time. If a new one is opened while another is active, the first is dismissed.

The returned text is always a string. Convert it to a number with `parseInt()`, `parseFloat()`, or `Engine.getValueForText()` for custom mode parsing (e.g. frequency or tempo names).

| Property | Type | Default | Description |
| --- | --- | --- | --- |
| `parentComponent` | String | none | ID of the parent component |
| `text` | String | `""` | Initial text content |
| `x`, `y`, `width`, `height` | int | none | Position and size relative to parent. If empty, centres on the component. |
| `bgColour` | int/hex | `0x88000000` | Background colour |
| `itemColour` | int/hex | `0` | Outline colour |
| `textColour` | int/hex | `0xAAFFFFFF` | Text colour |
| `alignment` | String | `"centred"` | Text alignment |
| `fontName` | String | `"Lato"` | Font name |
| `fontStyle` | String | `"plain"` | Font style (bold, italic, plain) |
| `fontSize` | float | `13.0` | Font size |

> [!Warning:Capture scope variables explicitly] The callback captures its enclosing scope at definition time. If you need to reference the parent component inside the callback, store it in a local variable and pass it via the capture list: `function[panel](ok, text) { ... }`.