<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# ScriptButton

ScriptButton is a binary on/off UI component created with `Content.addButton(name, x, y)`. It wraps a toggle button and operates on a fixed 0/1 value range that cannot be changed.

ScriptButton supports several interaction and rendering modes:

- **Toggle mode** (default) - click to switch between on and off
- **Momentary mode** - turns on while the mouse is held down, returns to off on release
- **Radio groups** - assigning multiple buttons to the same `radioGroup` value enforces mutual exclusion so only one button in the group can be on at a time
- **Filmstrip rendering** - set the `filmstripImage` property to render the button from a sprite sheet instead of the default skin
- **Popup attachment** - use `setPopupData()` to show a FloatingTile popup when the button is clicked

Filmstrip images support two frame layouts:

| numStrips | Frame Layout |
|-----------|-------------|
| 2 | Frame 0 = off, Frame 1 = on |
| 6 | Frames 0-1 = normal off/on, 2-3 = pressed off/on, 4-5 = hover off/on |

Only 2 and 6 are valid strip counts. Other values silently fall back to the default skin. A custom look and feel set via `setLocalLookAndFeel()` takes priority over filmstrip rendering.

The `setValueOnClick` property controls whether the button responds on mouse-down (true) or mouse-up (false, the default), giving immediate response when needed.

> [!Tip:Inherits most methods from ScriptComponent] ScriptButton inherits most of its methods from ScriptComponent. The button-specific methods are `setPopupData()` for popup attachment and the filmstrip/radio group/momentary properties. The `min` and `max` properties are deactivated since the value range is always 0 to 1. When a radio group button is exposed as a plugin parameter, it is automatically flagged as a meta parameter because toggling one button affects others in the group.

## Common Mistakes

- **Use filmstripImage property for filmstrips**
  **Wrong:** `btn.set("numStrips", 4)`
  **Right:** `btn.set("numStrips", 2)` or `btn.set("numStrips", 6)`
  *Only 2-strip and 6-strip filmstrip modes are supported. Other values silently fall back to the default skin without any error message.*

- **Position array needs exactly four elements**
  **Wrong:** `btn.setPopupData(jsonData, [10, 20])`
  **Right:** `btn.setPopupData(jsonData, [10, 20, 300, 200])`
  *The position parameter must be a 4-element array `[x, y, w, h]` specifying offset and popup dimensions. Incomplete arrays throw a script error.*

- **Avoid setValueWithUndo in control callbacks**
  **Wrong:** Calling `setValueWithUndo()` from inside `onControl`
  **Right:** Use `setValueWithUndo()` only for user-initiated actions outside the control callback
  *Calling `setValueWithUndo()` from `onControl` creates a feedback loop because the undo operation itself triggers the control callback again.*
