<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# TableProcessor

TableProcessor provides script-level access to the lookup table curves embedded in HISE modules such as velocity modulators, table envelopes, key modulators, and waveshaping effects. Any module that uses a table graph for its transfer function can be controlled through this interface.

You can build custom curves programmatically for:

- Dynamics crossfade shapes across velocity or dynamics layers
- Key range gating (bandpass curves that restrict a modulator to a note range)
- Articulation envelope shaping (rebuilding attack/release contours at runtime)

The typical workflow is to reset the table to a clean state, add interior points if needed, then adjust each point's position and curve. For simple two-point shapes (fade-in, fade-out), resetting and calling `setTablePoint()` on the two default edge points is sufficient. For more complex curves, add interior points first with `addTablePoint()`, then shape them with `setTablePoint()`.

```js
const var tp = Synth.getTableProcessor("VelocityMod");
```

You can also convert an existing Modulator reference with `Modulator.asTableProcessor()`, which returns `undefined` if the modulator has no table.

The `curve` parameter on table points controls interpolation between adjacent points: `0.5` produces linear interpolation, values below `0.5` produce a concave shape (equal-power fade-out), and values above `0.5` produce a convex shape (equal-power fade-in).

Most modules have a single table at index 0. Pass a higher `tableIndex` only when the module explicitly provides multiple tables.

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

> [!Tip:Get reference in onInit, use Table for bulk ops] `Synth.getTableProcessor()` can only be called during `onInit`. Store the reference in a `const var` at the top level of your script. For bulk point operations, retrieve the `Table` data object via `getTable()` and work with it directly - it avoids per-point UI update overhead.

## Common Mistakes

- **Cache processor reference in onInit**
  **Wrong:** `var tp = Synth.getTableProcessor("VeloMod");` in `onNoteOn`
  **Right:** `const var tp = Synth.getTableProcessor("VeloMod");` in `onInit`
  *`getTableProcessor()` can only be called during `onInit`. Store the reference as a top-level const variable.*

- **Call reset before rebuilding table**
  **Wrong:** Calling `addTablePoint()` without `reset()` first
  **Right:** Always call `reset()` before building a new curve shape
  *Without reset, new points accumulate on top of existing ones, producing an unpredictable curve. Every curve-building sequence should start with `reset()`.*

- **Use setTablePointsFromArray for bulk updates**
  **Wrong:** Many `addTablePoint()` calls in a loop for bulk curve building
  **Right:** Use the `Table` object from `getTable()` for batch operations
  *Each point modification through TableProcessor triggers an individual UI update. For bulk operations, work with the Table data object directly.*
