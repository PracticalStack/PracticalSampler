# ScriptTable -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md`
- `enrichment/resources/survey/class_survey_data.json`
- `enrichment/phase1/Content/Readme.md` (prerequisite)
- `enrichment/resources/explorations/ScriptComponent_base.md`
- `enrichment/resources/explorations/ScriptAudioWaveform.md` (used for shared ComplexDataScriptComponent context)
- No `ComplexDataScriptComponent_base.md` exists in `enrichment/resources/explorations/` at time of writing.

## Source Files

- Header declaration: `HISE/hi_scripting/scripting/api/ScriptingApiContent.h:1422`
- Main implementation: `HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350`
- JUCE wrapper: `HISE/hi_scripting/scripting/api/ScriptComponentWrappers.h:645`
- JUCE wrapper implementation: `HISE/hi_scripting/scripting/api/ScriptComponentWrappers.cpp:1571`
- Shared complex-data base: `HISE/hi_scripting/scripting/api/ScriptingApiContent.h:1358`, `HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3137`
- Table data model: `HISE/hi_tools/hi_tools/Tables.h:50`, `HISE/hi_tools/hi_tools/Tables.cpp:35`
- Table editor behavior: `HISE/hi_tools/hi_standalone_components/TableEditor.h:83`, `HISE/hi_tools/hi_standalone_components/TableEditor.cpp:36`
- Script-side table data handle: `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:1250`, `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp:2084`
- External data dispatch: `HISE/hi_dsp_library/snex_basics/snex_ExternalData.h:595`, `HISE/hi_dsp_library/snex_basics/snex_ExternalData.cpp:111`
- Dynamic external data provider: `HISE/hi_core/hi_dsp/ProcessorInterfaces.h:167`, `HISE/hi_core/hi_dsp/ProcessorInterfaces.cpp:449`

## Prerequisite Context Integration (Content)

The `Content` prerequisite model applies directly:

- `ScriptTable` is created via `Content.addTable()` and follows the same onInit-time creation lifecycle.
- It does not implement a custom factory path. It uses Content's generic `addComponent<ScriptTable>()` path.
- Property and callback semantics are inherited from `ScriptComponent` and then specialized by `ComplexDataScriptComponent` and `ScriptTable`.
- This class specialization is not "new UI lifecycle" infrastructure, but "table-specific complex-data binding + editor behavior" layered on top of Content/ScriptComponent.

## Survey Data (ScriptTable entry)

From `class_survey_data.json`:

- `createdBy`: `Content`
- `creates`: none
- `seeAlso`:
  - `ScriptSliderPack` (discrete step array editing)
  - `ScriptAudioWaveform` (audio sample visualization/edit interaction)
  - `Table` (data object behind ScriptTable UI)

## Inheritance and Internal Class Topology

```text
ScriptComponent
  -> ComplexDataScriptComponent
      -> ScriptTable
```

`ScriptTable` also participates in wrapper-side composition:

```text
ScriptTable (script object)
  <-> ScriptCreatedComponentWrappers::TableWrapper
        owns TableEditor (JUCE component)
```

Key split:

- `ScriptTable` stores API state (`snapValues`, `tableValueFunction`, dragProperties broadcaster)
- `TableWrapper` applies script properties to live JUCE `TableEditor`
- `TableEditor` performs actual user interaction, drag logic, curve calculation, undo integration
- `Table` / `SampleLookupTable` holds and interpolates graph data

## Class Declaration Details

Declaration in `ScriptingApiContent.h`:

- Class: `struct ScriptTable : public ComplexDataScriptComponent`
- Enum `Properties`:
  - `TableIndex = ScriptComponent::Properties::numProperties`
  - `customColours`
  - `numProperties`
- Index binding override: `getIndexPropertyId() const override { return TableIndex; }`
- Added API methods declared in class:
  - `setTablePopupFunction(var)`
  - `connectToOtherTable(String, int)`
  - `reset()`
  - `addTablePoint(float, float)`
  - `setTablePoint(int, float, float, float)`
  - `getTableValue(float)`
  - `setSnapValues(var)`
  - `referToData(var)`
  - `registerAtParent(int)`
  - `setMouseHandlingProperties(var)`
- Internal state:
  - `LambdaBroadcaster<var> dragProperties`
  - `var snapValues`
  - `var tableValueFunction`

No listener/target nested class exists inside `ScriptTable` itself. Listener-style behavior is delegated to `TableWrapper` implementing `TableEditor::EditListener`.

## Shared ComplexDataScriptComponent Infrastructure Used by ScriptTable

Relevant inherited infrastructure:

1) Data object ownership and default source:

```cpp
ownedObject = snex::ExternalData::create(type);
```

For `ScriptTable`, `type` is `ExternalData::DataType::Table`, so default data source is an internally owned `Table` object.

2) Source selection chain in `getUsedData()`:

- If `otherHolder` is set (via `referToDataBase`), use `otherHolder->getComplexBaseType(type, externalIndex)`
- Else if connected processor implements `ExternalDataHolder`, use processor slot (`processorId` + index property)
- Else use local `ownedObject`

3) Automatic rebinding on property changes:

- `setScriptObjectPropertyWithChangeMessage` in base calls `updateCachedObjectReference()` when either:
  - `processorId` changes
  - index property (`TableIndex`) changes

4) Event/listener bridge:

- `updateCachedObjectReference()` removes/adds event listener on `ComplexDataUIUpdater`
- updates `sourceWatcher` so wrappers can react and rebind editors

5) Serialization:

- Base export stores `"data" = cachedObjectReference->toBase64String()`
- Base restore reads same base64 back via `fromBase64String`

6) Dynamic registration with script processor:

- `registerComplexDataObjectAtParent(index)` calls `ProcessorWithDynamicExternalData::registerExternalObject`
- returns typed script handle for data type:
  - for Table: `new ScriptingObjects::ScriptTableData(...)`

## ScriptTable Constructor and Registration

Implementation in `ScriptingApiContent.cpp:3374`:

```cpp
ScriptTable(...):
    ComplexDataScriptComponent(base, name, snex::ExternalData::DataType::Table)
```

### Properties added

- `tableIndex` (script property id for external table slot)
- `customColours` (toggle selector)

### Defaults

- `x = x`
- `y = y`
- `width = 100`
- `height = 50`
- `TableIndex = 0`
- `customColours = 0`

### Lifecycle init sequence

1. `handleDefaultDeactivatedProperties()`
2. `initInternalPropertyFromValueTreeOrDefault(processorId)`
3. `initInternalPropertyFromValueTreeOrDefault(TableIndex)`
4. `updateCachedObjectReference()`
5. API method registrations

### API method registrations (class-local)

All are untyped `ADD_API_METHOD_N` (no forced var types declared at this layer):

- `reset`
- `addTablePoint`
- `setTablePoint`
- `getTableValue`
- `connectToOtherTable`
- `setSnapValues`
- `referToData`
- `setTablePopupFunction`
- `registerAtParent`
- `setMouseHandlingProperties`

Wrapper method table matches these 10 registrations.

Important: base JSON method list for ScriptTable does not include `connectToOtherTable`, even though C++ registers it.

## Deactivated Property Pattern

`ScriptTable::handleDefaultDeactivatedProperties()`:

- Starts with `ComplexDataScriptComponent::handleDefaultDeactivatedProperties()`
- Adds deactivation of:
  - `max`
  - `min`
  - `defaultValue`
  - `textColour`

This means ScriptTable intentionally hides value-range and text-colour semantics that are meaningful for generic numeric controls.

## Wrapper Topology and Responsibilities

`TableWrapper` constructor:

```cpp
TableEditor *t = new TableEditor(mc->getControlUndoManager(), table->getTable(0));
table->dragProperties.addListener(*t, [](TableEditor& te, const var& p)
{
    te.setMouseDragProperties(p);
});
table->getSourceWatcher().addSourceListener(this);
```

Implications:

- Wrapper uses control UndoManager from `MainController`.
- It initializes editor with current table pointer (`table->getTable(0)`).
- It subscribes to ScriptTable drag-property broadcasts.
- It subscribes to source changes from ComplexData source watcher.

`ScriptCreatedComponentWrapper::updateIfComplexDataProperty()` checks:

- `processorId`
- type-specific index (`TableIndex`)

When one changes, wrapper calls `updateComplexDataConnection()` which passes new `ComplexDataUIBase` pointer into editor (`EditorBase::setComplexDataUIBase`).

## Property-to-Behavior Mapping in TableWrapper

`TableWrapper::updateComponent(int propertyIndex, var newValue)`:

| Property | Behavior in TableEditor |
|---|---|
| `bgColour` | sets `TableEditor::bgColour` |
| `itemColour` | sets `TableEditor::fillColour` |
| `itemColour2` | sets `TableEditor::lineColour` |
| `tooltip` | sets tooltip |
| `customColours` | `setUseFlatDesign(newValue)` |
| `parameterId` | `setSnapValues(st->snapValues)` |

The `parameterId` case is an intentional routing hack used by ScriptTable methods to trigger UI-side updates for snap values and popup-function behavior.

## Table Popup Text Function Pipeline

Flow:

1. Script calls `ScriptTable.setTablePopupFunction(var newFunction)`
2. ScriptTable stores `tableValueFunction = newFunction`
3. ScriptTable calls `getPropertyValueTree().sendPropertyChangeMessage(parameterId)`
4. Wrapper receives property change and runs `t->setSnapValues(st->snapValues)` (same trigger path)
5. During drag, `TableWrapper::getTextForTablePopup(x, y)` checks if `tableValueFunction` is JS function
6. If valid, calls `callExternalFunction` with args `[x, y]`; otherwise falls back to `TableEditor::getPopupString`

Default fallback popup string is from `TableEditor`:

```cpp
editedTable->getXValueText(x) + " | " + editedTable->getYValueText(y)
```

## setMouseHandlingProperties Schema (JSON-ish var object)

`ScriptTable.setMouseHandlingProperties(var propertyObject)` broadcasts to wrapper; wrapper passes object to `TableEditor::setMouseDragProperties`; parser is `MouseDragProperties::fromVar`.

Recognized keys:

- `syncStartEnd` (bool)
- `allowSwap` (bool)
- `fixLeftEdge` (float)
- `fixRightEdge` (float)
- `snapWidth` (float)
- `numSteps` (int)
- `midPointSize` (int)
- `dragPointSize` (int)
- `endPointSize` (int)
- `useMouseWheelForCurve` (bool)
- `margin` (float)
- `closePath` (bool)

Behavioral consequences:

- `numSteps != -1`: rebuilds snap grid to evenly spaced points [0..1] (including explicit 1.0)
- `fixLeftEdge > -0.5`: forces first point to `(x=0.0, y=fixLeftEdge)`
- `fixRightEdge > -0.5`: forces last point to `(x=1.0, y=fixRightEdge)`
- both fixed: calls `Table::setStartAndEndY(left, right)` for endpoint-lock path behavior
- `allowSwap` false: dragged points are x-limited between neighbors (no crossing)
- `allowSwap` true: free crossing inside editor area (except hard margins)
- `syncStartEnd` true: dragging one edge point mirrors y to opposite edge
- `closePath` false: path draw/shape does not close area fill

## Snap Value Pipeline (setSnapValues)

Script API path:

```cpp
void ScriptTable::setSnapValues(var snapValueArray)
{
    if (!snapValueArray.isArray())
        reportScriptError("You must call setSnapValues with an array");

    snapValues = snapValueArray;
    getPropertyValueTree().sendPropertyChangeMessage(getIdFor(parameterId));
}
```

UI behavior in TableEditor:

- Accepts array values converted to float snap positions
- Snap width used as tolerance window around each snap point
- x drag is quantized only when pointer enters a snap window

Note: even if non-array is passed and script error is reported, assignment still runs and property change still fires. Wrapper side then calls `TableEditor::setSnapValues`, which ignores non-array input and keeps old snap list.

## Method Infrastructure Notes (non-line-by-line)

- `reset`, `addTablePoint`, `setTablePoint` directly delegate to cached `Table` object.
- `getTableValue` requires `dynamic_cast<SampleLookupTable*>`; returns `0.0f` if cast fails.
- `referToData` delegates to shared `referToDataBase` type-dispatch logic.
- `registerAtParent` delegates to shared complex-data registration.
- `connectToOtherTable` sets `processorId` and `TableIndex`; this triggers rebind through property/message chain.

## Upstream Data Provider Chain (Provider -> Dependency -> ScriptTable)

### Chain A: Internal data mode (no external binding)

1. `ScriptTable` constructor creates `ownedObject = ExternalData::create(Table)`
2. `ownedObject` is cached by `ComplexDataScriptComponent::getUsedData`
3. `TableWrapper` binds `TableEditor` to cached data
4. Script API methods mutate/inspect this owned `Table`

### Chain B: Processor-bound mode

1. Script sets `processorId` and/or `TableIndex`
2. `ComplexDataScriptComponent::getExternalHolder()` resolves connected processor as `ExternalDataHolder`
3. `getUsedData` pulls `eh->getComplexBaseType(Table, index)`
4. In dynamic processors, this resolves through `ProcessorWithDynamicExternalData::tables`
5. `TableEditor` rebinds through source watcher and wrapper update

### Chain C: Explicit data-object mode (`referToData`)

1. Script passes `ScriptTableData` handle or another complex-data component
2. `referToDataBase` stores `otherHolder` and index
3. `getUsedData` resolves table from that holder, overriding processor/default mode
4. Wrapper rebinds editor to new source

### Chain D: Dynamic registration outward (`registerAtParent`)

1. Script calls `registerAtParent(index)`
2. `registerComplexDataObjectAtParent` requires `ProcessorWithDynamicExternalData`
3. Calls `registerExternalObject(Table, index, ownedObject)`
4. Returns `ScriptTableData` object bound to same slot
5. Other systems (scriptnode, other scripts/components) can access same table slot via holder APIs

No backend/frontend divergence is present in this runtime data path. It is shared infrastructure.

## Enum / Constant Behavioral Tracing

ScriptTable itself has no `addConstant()` constructor constants. Behavioral selectors are property values / JSON keys.

### ScriptTable::Properties selector behavior

| Selector | Consumption point | Behavioral effect |
|---|---|---|
| `TableIndex` | `ComplexDataScriptComponent::setScriptObjectPropertyWithChangeMessage` and `getUsedData` | Changes which table slot is bound from external holder. Triggers source rebind and editor data pointer swap. |
| `customColours` | `TableWrapper::updateComponent` | Toggles `TableEditor::setUseFlatDesign`, changing look-and-feel drawing style path. |

### Drag property selectors (setMouseHandlingProperties)

| Key | Decision points | Runtime behavior |
|---|---|---|
| `allowSwap` | `TableEditor::mouseDrag` | True allows point x-crossing; false constrains x between neighboring points. |
| `syncStartEnd` | `TableEditor::mouseDrag` | True mirrors y-change from one edge point to the opposite edge. |
| `fixLeftEdge` / `fixRightEdge` | `TableEditor::setMouseDragProperties`, `DragPoint::canBeModified` | Locks edge y-values and can disable edge dragging depending on configured threshold. |
| `numSteps` | `TableEditor::setMouseDragProperties` | Replaces snap list with evenly spaced quantization grid. |
| `snapWidth` | `TableEditor::snapXValueToGrid` | Controls snap capture window width around snap points. |
| `closePath` | `TableEditor::refreshGraph` + `Table::createPath` | Controls whether table fill/path closes to baseline or remains open. |

### setTableValue path selector

`getTableValue()` only returns interpolated data when cached object is `SampleLookupTable` (dynamic_cast check). If another `Table` subtype is bound, method returns `0.0f`.

## Threading / Lifecycle Constraints

- Creation path is Content-driven, so component must be created in onInit phase.
- Runtime editing and callbacks are UI/message-thread centric (`TableEditor`, async popup updates, property messages).
- `getTableValue` calls `SampleLookupTable::getInterpolatedValue(..., sendNotificationAsync)`, which emits display update notification asynchronously.
- `Table` internals use `SimpleReadWriteLock` around graph point operations.

## Preprocessor Guards Relevant to ScriptTable Context

- No direct `#if` guard around ScriptTable API methods or wrapper behavior.
- Related object `ScriptTableData::createPopupComponent` has `#if USE_BACKEND` for backend popup editor creation only; returns null in non-backend builds.

## Interface Compatibility and Callback Infrastructure

- ScriptTable inherits callback and object infrastructure through ScriptComponent, including WeakCallbackHolder-based control callback systems.
- ScriptTable-specific popup callback (`tableValueFunction`) is invoked via script engine external function call path in wrapper, not via the ScriptComponent control callback pipeline.
- Drag interaction uses `TableEditor::EditListener` interface implemented by wrapper (`pointDragStarted`, `pointDragged`, `curveChanged`, `pointDragEnded`) for value popup behavior.

## Factory and obtainedVia

C++ factory method:

```cpp
ScriptingApi::Content::ScriptTable * ScriptingApi::Content::addTable(Identifier labelName, int x, int y)
{
    return addComponent<ScriptTable>(labelName, x, y);
}
```

Script obtained via `Content.addTable(name, x, y)`.

## Notable Structural Mismatch to Track in Step B

- C++ registers `connectToOtherTable(String, int)` for ScriptTable.
- Base JSON method list for `ScriptTable` does not currently include `connectToOtherTable`.
- Step B method checklist should still follow base JSON authoritative list, but this mismatch is important architectural context.
