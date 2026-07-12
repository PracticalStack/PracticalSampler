# TableProcessor -- Class Analysis

## Brief
Script handle for editing lookup table curves in modulators and effects via point manipulation and serialization.

## Purpose
`TableProcessor` provides script-level access to the lookup tables embedded in HISE modules such as velocity modulators, table envelopes, key modulators, and waveshaping effects. It wraps any `Processor` that implements the `ExternalDataHolder` interface (typically via `LookupTableProcessor`), exposing methods to add, modify, and reset table graph points using normalized coordinates, serialize/restore table state as base64 strings, and extract `Table` complex data objects for richer interaction. Most table-bearing modules have a single table (index 0), but the `tableIndex` parameter supports modules with multiple tables.

## obtainedVia
`Synth.getTableProcessor(processorId)` (onInit only, owner-rooted search) | `Modulator.asTableProcessor()` (returns undefined if the modulator has no table) | `Builder.get(id, "TableProcessor")`

## minimalObjectToken
tp

## Constants
None. TableProcessor has no static `addConstant()` calls.

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| (processor parameters) | int | Each parameter of the wrapped processor is registered as a named constant mapping to its parameter index. The available constants depend on the module type (e.g., a TableEnvelope exposes Attack, Release; a VelocityModulator exposes its parameters). These constants are for use with the underlying processor's parameter system, not for TableProcessor's own API methods. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var tp = Synth.getTableProcessor("VeloMod");` in `onNoteOn` | `const var tp = Synth.getTableProcessor("VeloMod");` in `onInit` | getTableProcessor() can only be called in onInit. Store the reference as a top-level const variable. |
| `tp.addTablePoint(0, 0.5, 0.8); tp.addTablePoint(0, 0.7, 0.3);` (many calls in a loop) | Use `Table` object from `tp.getTable(0)` for bulk operations | Multiple point modifications through TableProcessor trigger individual UI updates. For batch operations, work with the Table data object directly. |

## codeExample
```javascript
// Obtain a reference to a table-bearing module in onInit
const var tp = Synth.getTableProcessor("VelocityMod");

// Add a point at the center of the table
tp.addTablePoint(0, 0.5, 0.75);

// Or get the Table data object for richer access
const var tableData = tp.getTable(0);
```

## Alternatives
- `Modulator` -- controls modulator parameters and intensity without direct table access; use `Modulator.asTableProcessor()` to get a TableProcessor from it
- `Table` -- the data object itself (returned by `getTable()`), provides point editing plus table evaluation and display callbacks

## Related Preprocessors
None.

## Diagrams

### complex-table-data-chain
- **Brief:** Table Data Chain
- **Type:** topology
- **Description:** Table workflows use a three-part chain. `TableProcessor` selects the processor that owns one or more table slots, `Table` exposes the complex data stored in a specific slot, and `ScriptTable` displays or edits that same slot in the UI. The binding pair is `processorId` plus `tableIndex`, which is a complex-data connection and not the normal `parameterId` parameter binding path.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods have straightforward error handling via null checks on the underlying table. There are no timeline dependencies, no silent-failure preconditions, and no mode-dependent behavior that warrants parse-time diagnostics.
