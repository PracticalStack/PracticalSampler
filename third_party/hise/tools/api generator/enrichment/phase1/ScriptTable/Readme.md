# ScriptTable -- Class Analysis

## Brief
Table curve editor component with complex-data binding, drag customization, and external table linking.

## Purpose
ScriptTable is the UI-side table editor created by `Content.addTable()`. It specializes the shared `ComplexDataScriptComponent` layer for `ExternalData::Table` and connects scripting calls to a live `TableEditor` wrapper. The class supports internal table ownership, processor-slot binding, and explicit data-object binding via `referToData`. It also exposes drag behavior customization and popup text override hooks used by the editor interaction layer.

## Details

### Architecture Layering

ScriptTable sits on top of two prerequisite layers:

- `Content` provides factory lifecycle and component creation (`addTable`).
- `ScriptComponent` provides shared component methods/properties/callback plumbing.

ScriptTable then adds a complex-data binding layer:

- `ComplexDataScriptComponent` resolves table source (`ownedObject`, processor holder, or referred holder).
- `ScriptTable` stores table-specific script state (`snapValues`, popup function, drag-property broadcaster).
- `TableWrapper` and `TableEditor` implement runtime UI behavior and user drag interactions.

### Data Source Modes

| Mode | How selected | Source |
|------|--------------|--------|
| Internal | default | internally owned `Table` object |
| Processor slot | `processorId` + `tableIndex` | external holder table slot |
| Referred object | `referToData(tableData or other component)` | holder from referenced object |

`registerAtParent(index)` registers the internal table into a dynamic external-data holder and returns a `Table` data handle object (`ScriptTableData`). For binding workflows and accepted source forms, see `registerAtParent()` and `referToData()`.

### ScriptTable-specific Property Effects

| Property | Effect |
|----------|--------|
| `tableIndex` | Selects external table slot when bound to holder/processor |
| `customColours` | Toggles flat-design rendering path in `TableEditor` |

ScriptTable also deactivates inherited range/text properties (`min`, `max`, `defaultValue`, `textColour`) because table editing is point-graph based, not scalar-range based.

### Mouse Handling Property Object

`setMouseHandlingProperties(object)` configures `TableEditor::MouseDragProperties` for edge locking, quantization, and drag rendering. See `setMouseHandlingProperties()` for the full key list, value constraints, and examples.

### Popup Text Override Path

`setTablePopupFunction(function)` installs a drag-popup formatter callback used by the wrapper layer. See `setTablePopupFunction()` for callback signature, fallback behavior, and usage example.

## obtainedVia
`Content.addTable(componentId, x, y)`

## minimalObjectToken
st

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `st.setSnapValues(0.25);` | `st.setSnapValues([0.0, 0.25, 0.5, 0.75, 1.0]);` | `setSnapValues` expects an array. Non-array input triggers a script error and does not build snap points. |
| `st.referToData(sliderPackDataObj);` | `st.referToData(tableDataObj);` | `referToData` enforces matching complex-data type (`Table`). Passing another data type reports "Data Type mismatch" and does not bind. |
| `st.setKeyPressCallback(function(event){});` before configuring consumed keys | `st.setConsumedKeyPresses("all");` then `st.setKeyPressCallback(function(event){});` | Inherited key callback contract requires consumed-key configuration first, otherwise runtime reports an error. |

## codeExample
```javascript
const var st = Content.addTable("EnvCurve", 20, 20);
st.setTablePoint(1, 0.5, 0.8, 0.5);
st.setMouseHandlingProperties({ "allowSwap": false, "numSteps": 8 });
```

## Alternatives
- `ScriptSliderPack` -- use for discrete step arrays instead of continuous curve editing.
- `ScriptAudioWaveform` -- use for audio sample display/range interaction instead of parameter transfer curves.
- `Table` -- use when you need direct data-handle operations without UI component interaction.

## Related Preprocessors
`USE_BACKEND` (indirectly relevant for `ScriptTableData` popup editor creation only). ScriptTable component behavior itself has no class-local preprocessor gating.

## Diagrams

### complex-table-data-chain
- **Brief:** Table Data Chain
- **Type:** topology
- **Description:** Table workflows use a three-part chain. `TableProcessor` selects the processor that owns one or more table slots, `Table` exposes the complex data stored in a specific slot, and `ScriptTable` displays or edits that same slot in the UI. The binding pair is `processorId` plus `tableIndex`, which is a complex-data connection and not the normal `parameterId` parameter binding path.

## Diagnostic Ideas
Reviewed: Yes
Count: 4
- ScriptTable.referToData -- value-check (logged)
- ScriptTable.registerAtParent -- precondition (logged)
- ScriptTable.setMouseHandlingProperties -- value-check (logged)
- ScriptTable.setTablePopupFunction -- value-check (logged)
