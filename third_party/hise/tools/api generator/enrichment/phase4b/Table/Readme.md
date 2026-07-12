Table (object)
Obtain via: Engine.createAndRegisterTableData(index) | TableProcessor.getTable(tableIndex) | ScriptTable.registerAtParent(index)

Editable lookup table data object for shaping modulation curves via control points.
Stores graph points with normalized x/y coordinates and a curve factor, rendered into
a 512-element float lookup array for interpolated value queries. Used for velocity
response, envelope shaping, and waveshaping throughout HISE.

Complexity tiers:
  1. UI-driven curve query: ScriptTable UI + registerAtParent(0) to get a Table handle,
     then getTableValueNormalised() in a MIDI callback. User edits the curve visually.
  2. Programmatic curve setup: + setTablePointsFromArray, reset. Define curves from code
     (loading presets, generating mathematical shapes).
  3. Linked tables with callbacks: + linkTo, setContentCallback, setDisplayCallback.
     Shared data between multiple Table handles with reactive UI synchronization.

Practical defaults:
  - Use ScriptTable + registerAtParent(0) when the curve should be user-editable.
    Use Engine.createAndRegisterTableData() when purely programmatic with no UI.
  - The default linear ramp (0,0) to (1,1) after reset() is identity -- input equals
    output. Good starting point for velocity curves.
  - Normalize MIDI values to 0.0-1.0 before querying:
    table.getTableValueNormalised(Message.getVelocity() / 127.0), then scale back.

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
  - Passing raw MIDI velocity (0-127) to getTableValueNormalised() -- any input above
    1.0 returns the last table value, flattening the response. Normalize to 0.0-1.0.
  - Calling addTablePoint() in a loop -- each call triggers a full 512-element lookup
    table re-render. Use setTablePointsFromArray() for bulk setup.
  - Passing fewer than 2 points to setTablePointsFromArray() -- triggers a script error.
    A table requires at least 2 points.
  - Expecting edge point x values to be preserved in setTablePointsFromArray() -- first
    point x is forced to 0.0, last point x to 1.0 regardless of values passed.

Example:
  // Create a standalone table data object
  const var td = Engine.createAndRegisterTableData(0);

  // Set up a custom curve
  td.setTablePointsFromArray([
      [0.0, 0.0, 0.5],
      [0.3, 0.8, 0.3],
      [0.7, 0.2, 0.7],
      [1.0, 1.0, 0.5]
  ]);

  // Query a value from the curve
  var value = td.getTableValueNormalised(0.5);

Methods (10):
  addTablePoint                getCurrentlyDisplayedIndex
  getTablePointsAsArray        getTableValueNormalised
  linkTo                       reset
  setContentCallback           setDisplayCallback
  setTablePoint                setTablePointsFromArray
