# ScriptTable

<!-- Diagram triage:
  - none: CUT (no class-level or method-level diagram specifications in ScriptTable JSON)
-->

`ScriptTable` is a curve editor component for shaping normalised values with draggable points.

Use it when you want a user-editable transfer curve for velocity mapping, modulation shaping, or other response remapping where a single x-to-y lookup drives runtime behaviour. You can either create it in script with `Content.addTable(...)` or bind to an existing designed component with `Content.getComponent(...)`, then configure drag rules and data binding during `onInit`.

```js
const var st = Content.addTable("EnvCurve", 20, 20);
const var existing = Content.getComponent("Table1");
```

Typical ScriptTable workflows are:

1. Build one curve and query it with `getTableValue()` from note or control callbacks.
2. Register the table once with `registerAtParent()` and reuse the returned data handle in runtime code.
3. Apply a shared local look-and-feel created with `Content.createLocalLookAndFeel()`, then attach it with `setLocalLookAndFeel()` to keep multiple tables visually consistent.

### Interaction

Hold Ctrl and scroll the mouse wheel over a line segment to adjust its curve. This works for all table editors in HISE, including envelope displays.

Dragging table points does not trigger the control callback (`onControl` or `setControlCallback`). To run callback logic after a user edits the curve, call `changed()` explicitly from script or use the table data directly via `getTableValue()`.

### Styling

Set the `customColours` property to `true` to switch to flat-style rendering. This enables the standard colour properties (`bgColour`, `itemColour`, etc.) for table styling. Without this flag, colour properties have no effect on the table appearance.

## Complex Data Chain

Table workflows use a three-part complex-data chain:

![Table Data Chain](topology_complex-table-data-chain.svg)

- `TableProcessor` selects the module that owns one or more table slots.
- `Table` is the complex-data handle for one slot within that module.
- `ScriptTable` displays or edits one selected slot in the UI.

Use the binding properties separately:

- `processorId` selects the owning processor.
- `tableIndex` selects which table slot inside that processor should be displayed.

This is not the normal parameter binding path. `parameterId` targets processor parameters, while table-slot binding uses `tableIndex` instead.

`setSnapValues()` configures x-axis snap points for point dragging, while `setMouseHandlingProperties()` controls edit behaviour such as edge locking, swap rules, and grid stepping.

> [!Tip:Setup once in onInit, reuse handles] ScriptTable is a UI component. Do setup work once in `onInit` and reuse handles at runtime. Also note that `addToMacroControl()` is intentionally not active for ScriptTable, and `setPropertiesFromJSON()` should be done via `Content.setPropertiesFromJSON(componentId, jsonData)`.

## Common Mistakes

- **Normalise input to 0-1 range**
  **Wrong:** `table.getTableValue(Message.getVelocity())`
  **Right:** `table.getTableValue(Message.getVelocity() / 127.0)`
  *Table lookup input is normalised. Raw MIDI values collapse the curve response.*

- **Register at parent once at init time**
  **Wrong:** Calling `registerAtParent()` inside `onNoteOn`
  **Right:** Call `registerAtParent()` once in `onInit` and cache the returned handle
  *Registration is setup work, not per-note work. Repeating it during playback adds avoidable overhead.*

- **setSnapValues takes an array**
  **Wrong:** `st.setSnapValues(0.25)`
  **Right:** `st.setSnapValues([0.0, 0.25, 0.5, 0.75, 1.0])`
  *Snap values must be an array of x positions.*

- **referToData requires same data type**
  **Wrong:** `st.referToData(sliderPackDataObj)`
  **Right:** `st.referToData(tableDataObj)`
  *`referToData` only accepts table-compatible data sources.*

- **Override all table draw functions together**
  **Wrong:** Styling only one table draw function
  **Right:** Override the full set (`drawTableBackground`, `drawTablePath`, `drawTablePoint`, `drawTableRuler`)
  *Partial overrides often produce mixed default/custom rendering that looks inconsistent.*
