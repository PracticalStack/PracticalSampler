<!-- Diagram triage:
  - (no diagrams in Phase 1 data)
-->

# Table

Table is a data handle for an editable lookup curve used to shape modulation, velocity response, envelope curves, and waveshaping throughout HISE. It stores a set of control points with normalised x/y coordinates and a curve factor, which are rendered into a 512-element float array for efficient interpolated value queries.

You can obtain a Table handle in several ways:

```js
// From a ScriptTable UI component
const var table = ScriptTable.registerAtParent(0);

// Standalone (no UI)
const var td = Engine.createAndRegisterTableData(0);

// From a module's table slot
const var td = Synth.getComplexData("Table", "processorId", 0);
```

Each control point has three normalised (0.0-1.0) fields:

- **x** - horizontal position along the table
- **y** - output value at this position
- **curve** - interpolation shape between this point and the next (0.5 = linear)

The most common pattern is pairing a ScriptTable UI component with `getTableValueNormalised()` in a MIDI callback to remap velocity or modulation values through a user-editable curve. For programmatic curve setup without a UI, use `Engine.createAndRegisterTableData()` followed by `setTablePointsFromArray()`. You can also link multiple Table handles to share the same underlying data with `linkTo()`, and register callbacks to react to content or display changes.

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

> [!Tip:Edge points clamped to x boundaries] The first and last control points are always clamped to x=0.0 and x=1.0 respectively. You cannot move the edge points away from the table boundaries.

> [!Tip:getTableValueNormalised updates display ruler] Calling `getTableValueNormalised()` updates the display ruler position as a side effect, which fires any registered display callback and updates connected ScriptTable components.

## Common Mistakes

- **Use setTablePointsFromArray for bulk builds**
  **Wrong:** Calling `addTablePoint()` in a loop to build a curve
  **Right:** Use `setTablePointsFromArray()` with the complete point array
  *Each `addTablePoint()` call triggers a full 512-element lookup table re-render. `setTablePointsFromArray()` renders only once after all points are set.*

- **Include edge points at 0 and 1**
  **Wrong:** `setTablePointsFromArray([[0.5, 0.8, 0.5]])` with a single point
  **Right:** Always provide at least 2 points: `[[0.0, 0.0, 0.5], [1.0, 1.0, 0.5]]`
  *A table requires a minimum of 2 points. Passing fewer triggers a script error.*

- **Always include both edge points**
  **Wrong:** `setTablePointsFromArray([[0.3, 0.5, 0.5], [0.7, 1.0, 0.5]])` expecting x=0.3 as the start
  **Right:** The first point's x is always forced to 0.0, the last point's x to 1.0
  *Edge point x positions are silently clamped to the table boundaries regardless of the values you pass.*

- **Normalise input to 0-1 before lookup**
  **Wrong:** `table.getTableValueNormalised(Message.getVelocity())` passing raw 0-127
  **Right:** `table.getTableValueNormalised(Message.getVelocity() / 127.0)` normalising to 0.0-1.0
  *The method expects normalised 0.0-1.0 input. Raw MIDI values exceed 1.0 and return the last table value for all velocities above 1.*
