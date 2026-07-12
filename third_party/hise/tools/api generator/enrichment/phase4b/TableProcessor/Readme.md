TableProcessor (object)
Obtain via: Synth.getTableProcessor(processorId) | Modulator.asTableProcessor() | Builder.get(id, "TableProcessor")

Script handle for editing lookup table curves in modulators and effects.
Wraps any Processor implementing ExternalDataHolder (typically via
LookupTableProcessor) -- velocity modulators, table envelopes, key modulators,
waveshaping effects, and samplers. Most modules have one table (index 0).

Complexity tiers:
  1. Simple curve adjustment: reset, setTablePoint. Modify the two default edge
     points for a basic fade-in or fade-out shape.
  2. Multi-point curve building: + addTablePoint. Create interior points for
     bandpass shapes, crossfade segments, and custom envelope contours.
  3. Programmatic multi-table generation: Operating on multiple tables
     (tableIndex > 0) across arrays of TableProcessor references for
     complementary crossfade curve sets.

Practical defaults:
  - Always call reset() before addTablePoint() sequences to ensure a clean state.
  - Use curve 0.5 for linear, 0.25 for concave (equal-power fade-out), 0.75
    for convex (equal-power fade-in).
  - Most modules have a single table at index 0. Only use tableIndex > 0 when
    the module explicitly provides multiple tables.
  - For runtime adjustments (e.g., knob callbacks), call setTablePoint() on
    known indices rather than rebuilding the entire table.

Complex data chain:

![Table Data Chain](topology_complex-table-data-chain.svg)

  - TableProcessor selects the module that owns one or more table slots.
  - Table is the complex-data handle for one slot within that module.
  - ScriptTable displays or edits one selected slot in the UI.

  Use the binding properties separately:
  - processorId selects the owning processor.
  - tableIndex selects which table slot inside that processor should be displayed.

  This is not the normal parameter binding path. parameterId targets processor
  parameters, while table-slot binding uses tableIndex instead.

Common mistakes:
  - Calling addTablePoint() without reset() first -- new points accumulate on
    existing ones, producing unpredictable curves.
  - Calling Synth.getTableProcessor() outside onInit -- factory method is
    restricted to onInit. Store as a const var at the top level.
  - Calling Modulator.asTableProcessor() repeatedly in a callback -- creates a
    new wrapper each call. Cache the reference at init time.
  - Adding points between setTablePoint() calls -- point indices shift as
    points are added. Add all interior points first, then adjust with
    setTablePoint().

Example:
  // Obtain a reference to a table-bearing module in onInit
  const var tp = Synth.getTableProcessor("VelocityMod");

  // Add a point at the center of the table
  tp.addTablePoint(0, 0.5, 0.75);

  // Or get the Table data object for richer access
  const var tableData = tp.getTable(0);

Methods (7):
  addTablePoint       exists              exportAsBase64
  getTable            reset               restoreFromBase64
  setTablePoint
