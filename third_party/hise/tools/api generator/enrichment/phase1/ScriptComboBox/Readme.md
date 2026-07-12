# ScriptComboBox -- Class Analysis

## Brief
Drop-down list component for selecting from named items with dynamic item management and custom popup menus.

## Purpose
ScriptComboBox is a UI component that presents a drop-down list of named text items for single-selection. It uses 1-based indexing where value 1 corresponds to the first item. Items are stored as a newline-separated string and can be populated either through the `items` property or programmatically via `addItem()`. When `useCustomPopup` is enabled, the item list supports special formatting syntax for section headers, separators, submenus, and disabled entries.

## Details

### Value Model

ScriptComboBox uses **1-based integer indexing**:

| Value | Meaning |
|-------|---------|
| `0` | Nothing selected (placeholder text from `text` property is shown) |
| `1` | First item |
| `N` | Nth item |

The `min` property is fixed at `1` and the `max` property is auto-managed to equal the item count. Both are deactivated in the property editor. The `defaultValue` is `1` (first item selected).

Normalization (`setValueNormalized` / `getValueNormalized`) uses the base ScriptComponent implementation, which stores/returns the raw value without range mapping. This means `setValueNormalized(0.5)` calls `setValue(0.5)`, which is not useful for integer-indexed combo boxes. Use `setValue()` with explicit integer indices instead.

### Items Format

Items are stored as a newline-separated string in the `items` property. Setting items via `set("items", "Option A\nOption B\nOption C")` automatically updates `max` to match the item count.

### Custom Popup Syntax

When `useCustomPopup` is `true`, the items list supports special formatting:

| Syntax | Effect |
|--------|--------|
| `**HeaderText**` | Non-selectable section header |
| `___` | Non-selectable separator line |
| `Category::ItemName` | Item inside a submenu named "Category" |
| `Cat1::Cat2::Item` | Nested submenus |
| `~~DisabledItem~~` | Greyed-out disabled item |
| `ItemText\|` | Column break after this item |
| `%SKIP%` | Skip an index without adding a visible item |

Headers and separators do not consume selection indices. See `getItemText()` for how custom popup formatting affects index-to-text resolution.

### Color Mapping

| Property | Visual Role |
|----------|-------------|
| `bgColour` | Component outline |
| `itemColour` | Fill top gradient |
| `itemColour2` | Fill bottom gradient |
| `textColour` | Text color |

### Font Properties

The `fontName`, `fontSize`, and `fontStyle` properties control the combo box text appearance. Built-in font names `"Default"` and `"Oxygen"` map to the global HISE font. `"Source Code Pro"` maps to the global monospace font. Other names resolve through the MainController custom font registry, then fall back to system fonts.

### text Property as Placeholder

The inherited `text` property serves as the "nothing selected" placeholder text shown when the combo box value is 0.

## obtainedVia
`Content.addComboBox(name, x, y)`

## minimalObjectToken
cb

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `cb.setValue(0)` to select first item | `cb.setValue(1)` | ScriptComboBox uses 1-based indexing. Value 0 means "nothing selected" and shows placeholder text. |
| `cb.set("items", "A,B,C")` | `cb.set("items", "A\nB\nC")` | Items must be separated by newlines, not commas. Comma-separated text will appear as a single item. |

## codeExample
```javascript
const var cb = Content.addComboBox("MyComboBox", 0, 0);
cb.set("items", "Option A\nOption B\nOption C");
```

## Alternatives
- ScriptSlider -- for numeric value input with a continuous range instead of named items.
- ScriptButton -- for binary on/off toggling or radio groups instead of multi-option selection.
- ScriptedViewport -- for displaying and interacting with tabular data instead of simple drop-down selection.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ScriptComboBox is a straightforward UI component. The 1-based indexing model is the main footgun but it is standard across HISE combo boxes and produces clear runtime behavior (showing placeholder text) rather than silent failures. No parse-time diagnostics are warranted.
