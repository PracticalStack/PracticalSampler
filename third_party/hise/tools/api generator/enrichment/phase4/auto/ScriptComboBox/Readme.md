<!-- Diagram triage:
  - (no diagrams in Phase 1 data)
-->

# ScriptComboBox

ScriptComboBox is a drop-down list component created with `Content.addComboBox(name, x, y)` that lets the user select one item from a list of named options. It uses 1-based integer indexing: value 1 selects the first item, and value 0 means nothing is selected (the placeholder text from the `text` property is shown).

Items are stored as a newline-separated string in the `items` property. You can populate items statically at init or build them dynamically with `addItem()` after clearing the list. Setting `items` automatically updates the internal range to match the item count.

When `useCustomPopup` is enabled, the item string supports special formatting syntax:

| Syntax | Effect |
|--------|--------|
| `**HeaderText**` | Non-selectable section header |
| `___` | Non-selectable separator line |
| `Category::ItemName` | Item inside a submenu |
| `Cat1::Cat2::Item` | Nested submenus |
| `~~DisabledItem~~` | Greyed-out disabled item |

Headers and separators do not consume selection indices, so the value index counts only selectable items.

```js
const var cb = Content.addComboBox("MyComboBox", 0, 0);
cb.set("items", "Option A\nOption B\nOption C");
```

The four colour properties map to specific visual roles:

| Property | Visual Role |
|----------|-------------|
| `bgColour` | Component outline |
| `itemColour` | Fill top gradient |
| `itemColour2` | Fill bottom gradient |
| `textColour` | Text colour |

> [!Tip:Min and max managed automatically] The `min` and `max` properties are managed automatically and deactivated in the property editor. Do not set them manually. The normalised value methods (`setValueNormalized` / `getValueNormalized`) use the base implementation, which stores the raw value without range mapping - use `setValue()` with explicit integer indices instead.

> [!Tip:Additional methods in latest API] Several additional methods exist on this component that are not covered here: `createLocalLookAndFeel`, `getPopupMenuTarget`, `updateContentPropertyInternal`, `setColour`, and `getCurrentZLevel`. These are newer API additions; consult the latest HISE source for details.

## Common Mistakes

- **ComboBox values are 1-based**
  **Wrong:** `cb.setValue(0)` to select the first item
  **Right:** `cb.setValue(1)`
  *ScriptComboBox uses 1-based indexing. Value 0 means "nothing selected" and shows placeholder text.*

- **Use items property with newline separator**
  **Wrong:** `cb.set("items", "A,B,C")`
  **Right:** `cb.set("items", "A\nB\nC")`
  *Items must be separated by newlines, not commas. Comma-separated text appears as a single item.*

- **Subtract 1 for zero-based array index**
  **Wrong:** Using `value` directly as an array index
  **Right:** `array[parseInt(value) - 1]`
  *Combo box values arrive as 1-based floats in callbacks. Convert to integer and subtract 1 for 0-based array access.*

- **Clear items before repopulating**
  **Wrong:** Populating with `addItem()` without clearing first
  **Right:** `cb.set("items", ""); cb.addItem(...)`
  *The Interface Designer may have left stale items in the property. Clear before rebuilding.*

- **Store selection index not file list**
  **Wrong:** Saving dynamic file lists in presets
  **Right:** `cb.set("saveInPreset", false)`
  *File-scanned item lists differ between machines. Persist the file reference or processor state instead.*
