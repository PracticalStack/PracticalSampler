# Table -- Class Analysis

## Brief
Scriptable lookup table data object for editing modulation curves via control points with interpolated value queries.

## Purpose
`Table` is a complex data handle that wraps an editable lookup table (curve editor) used throughout HISE for shaping modulation, velocity response, envelope curves, and waveshaping. It stores a set of graph points with normalized x/y coordinates and a curve factor, which are rendered into a 512-element float lookup array for efficient interpolated value queries. Table objects can exist independently (created via `Engine.createAndRegisterTableData()`) or be obtained from module tables via `TableProcessor.getTable()`. The class supports display and content change callbacks for reactive UI updates, and can be linked to other Table instances to share underlying data.

## Details

### Architecture

Table is one of four complex data types in HISE (alongside SliderPackData, AudioFile, and DisplayBuffer). All share the `ScriptComplexDataReferenceBase` base class which provides:
- Event listener registration on the underlying data object's updater
- Display callback (fires when the ruler/playback position changes)
- Content callback (fires when points are added, removed, or modified)
- `linkTo()` for making two handles share the same underlying data

The scripting `Table` object wraps a C++ `SampleLookupTable` (the only active subclass of the abstract `Table` base), which maintains a fixed 512-float internal lookup array.

### Graph Points

Each table stores an array of `GraphPoint` structures with three normalized (0.0--1.0) fields:
- **x** -- horizontal position along the table
- **y** -- output value at this position
- **curve** -- curve factor controlling the interpolation shape between this point and the next (0.5 = linear)

The default state after `reset()` is two points: (0, 0, 0.5) and (1, 1, 0.5) -- a linear ramp.

### Lookup Table Rendering

When points are modified, the table re-renders its 512-float lookup array by:
1. Sorting graph points by x coordinate
2. Creating a JUCE Path from the points with curve interpolation
3. Walking the path with a PathFlatteningIterator to sample 512 evenly-spaced values

This rendering happens on every individual point modification. See `addTablePoint()`, `setTablePoint()`, and `setTablePointsFromArray()` for per-call re-render behavior and bulk alternatives.

### Edge Point Constraints

Edge points (first and last) are clamped to the table boundaries. See `setTablePoint()` and `setTablePointsFromArray()` for specific clamping rules.

### Value Query Side Effect

`getTableValueNormalised()` sends a display index notification as a side effect. See the method entry for details.

## obtainedVia
`Engine.createAndRegisterTableData(index)` | `TableProcessor.getTable(tableIndex)` | `Synth.getComplexData("Table", processorId, index)` | `ScriptTable.registerAtParent(index)`

## minimalObjectToken
td

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `addTablePoint()` in a loop for many points | Use `setTablePointsFromArray()` with the complete point array | Each `addTablePoint()` triggers a full 512-element lookup table re-render. `setTablePointsFromArray()` renders only once after all points are set. |
| `setTablePointsFromArray([[0.5, 0.8, 0.5]])` (single point) | Provide at least 2 points: `[[0.0, 0.0, 0.5], [1.0, 1.0, 0.5]]` | A table requires at least 2 points. Passing fewer triggers a script error. |
| `setTablePointsFromArray([[0.3, 0.5, 0.5], [0.7, 1.0, 0.5]])` expecting x=0.3 start | First point x is always forced to 0.0, last point x to 1.0 | Edge point x positions are automatically clamped to the table boundaries regardless of the values you pass. |

## codeExample
```javascript
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
```

## Alternatives
- `SliderPackData` -- use for discrete step-sequence or multi-slider data; Table is for continuous curve lookup data
- `AudioFile` -- use for audio waveform data from files; Table is for editable modulation curves defined by control points
- `DisplayBuffer` -- use for read-only ring buffer visualization; Table is for editable modulation curves
- `ScriptTable` -- the UI component that displays and edits a Table; Table is the data model for programmatic access without a UI
- `TableProcessor` -- module handle for accessing tables owned by processors in the signal chain; Table is the data object itself

## Related Preprocessors
`USE_BACKEND` -- createPopupComponent() for debugger inspection is backend-only.

## Diagrams

### complex-table-data-chain
- **Brief:** Table Data Chain
- **Type:** topology
- **Description:** Table workflows use a three-part chain. `TableProcessor` selects the processor that owns one or more table slots, `Table` exposes the complex data stored in a specific slot, and `ScriptTable` displays or edits that same slot in the UI. The binding pair is `processorId` plus `tableIndex`, which is a complex-data connection and not the normal `parameterId` parameter binding path.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Table methods have straightforward validation (point count checks, range clamping). There are no timeline dependencies, mode-dependent silent failures, or preconditions that warrant parse-time diagnostics. The two callback methods already have ADD_CALLBACK_DIAGNOSTIC registrations in C++.
