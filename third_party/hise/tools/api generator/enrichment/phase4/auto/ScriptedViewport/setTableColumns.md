Defines the columns of the table. Pass an array of column definition objects, each with at least an `ID` property that matches the keys in your row data. Must be called in onInit after `setTableMode()`.

**Common properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ID` | String | (required) | Column identifier, must match row data keys |
| `Type` | String | `"Text"` | Cell type: Text, Button, Slider, ComboBox, Hidden |
| `Label` | String | same as ID | Display name in the header |
| `Width` | int | - | Column width in pixels |
| `MinWidth` | int | 1 | Minimum column width |
| `MaxWidth` | int | -1 | Maximum column width (-1 for unlimited) |
| `Visible` | bool | true | Whether column is visible |
| `Focus` | bool | true | Whether column participates in arrow-key navigation |

**Button columns:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Toggle` | bool | false | false = momentary, true = toggle |
| `Text` | String | `"Button"` | Button label text |

**Slider columns:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `MinValue` | double | 0.0 | Range minimum |
| `MaxValue` | double | 1.0 | Range maximum |
| `StepSize` | double | 0.0 | Step size (0 = continuous) |
| `SkewFactor` | double | 1.0 | Skew (1.0 = linear) |
| `suffix` | String | `""` | Suffix text after the value |
| `defaultValue` | double | - | Value on double-click reset |
| `showTextBox` | bool | true | Enables shift-click text input |
| `style` | String | `"Knob"` | Knob, Horizontal, or Vertical |

> [!Tip:Range property naming convention] The range property names (`MinValue`, `MaxValue`, `StepSize`, `SkewFactor`) follow the default `"scriptnode"` naming convention. You can change this with the `SliderRangeIdSet` property in `setTableMode()`.

**ComboBox columns:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `items` | Array | - | Array of strings for the dropdown items |
| `ValueMode` | String | `"ID"` | ID (1-based int), Index (0-based int), or Text (string) |
| `Text` | String | `"No selection"` | Placeholder text |

For ComboBox columns, row data can override `items` and `Value` dynamically on a per-row basis. This is useful when different rows need different dropdown choices while sharing the same column definition.

> [!Warning:Disable Focus on non-navigable columns] Set `"Focus": false` on Button columns that should not participate in arrow-key navigation. Without this, the focus order includes every column, making keyboard navigation slow when there are many action buttons.
