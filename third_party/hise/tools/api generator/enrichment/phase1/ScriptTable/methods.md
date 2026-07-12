## addTablePoint

**Signature:** `undefined addTablePoint(Number x, Number y)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addTablePoint(0.5, 0.75);`

**Description:**
Adds a new graph point to the table data. The point is appended and the table is recalculated.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Number | no | Normalized x position for the new point | Intended range: 0.0-1.0 |
| y | Number | no | Normalized y value for the new point | Intended range: 0.0-1.0 |

**Pitfalls:**
- `addTablePoint()` does not clamp x / y to 0..1 in the underlying `Table` implementation, so out-of-range values can create unexpected curve shapes.

**Cross References:**
- `$API.ScriptTable.setTablePoint$`
- `$API.ScriptTable.reset$`

---

## getTableValue

**Signature:** `Number getTableValue(Number inputValue)`
**Return Type:** `Number`
**Call Scope:** unsafe
**Minimal Example:** `var y = {obj}.getTableValue(0.25);`

**Description:**
Returns the interpolated table output value for a normalized input position.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| inputValue | Number | no | Normalized lookup position | Intended range: 0.0-1.0 |

**Pitfalls:**
- [BUG] If the bound data source is missing or not a `SampleLookupTable`, this method silently returns `0.0`.

**Cross References:**
- `$API.ScriptTable.addTablePoint$`
- `$API.ScriptTable.setTablePoint$`

---

## setTablePoint

**Signature:** `undefined setTablePoint(Integer pointIndex, Number x, Number y, Number curve)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTablePoint(1, 0.5, 0.8, 0.5);`

**Description:**
Updates an existing graph point in the table.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pointIndex | Integer | no | Graph point index to edit | Must reference an existing point |
| x | Number | no | New normalized x value | Clamped to 0.0-1.0 (edge points keep x) |
| y | Number | no | New normalized y value | Clamped to 0.0-1.0 |
| curve | Number | no | Curve amount toward next point | Clamped to 0.0-1.0 |

**Pitfalls:**
- [BUG] Out-of-range `pointIndex` values are silently ignored.
- The first and last points keep fixed x positions (`0` and `1`) even if another x value is supplied.

**Cross References:**
- `$API.ScriptTable.addTablePoint$`
- `$API.ScriptTable.getTableValue$`

---

## setTablePopupFunction

**Signature:** `undefined setTablePopupFunction(Function newFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTablePopupFunction(onTablePopupText);`

**Description:**
Sets a custom function for drag-popup text formatting. The function receives `(x, y)` and should return display text.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newFunction | Function | no | Popup text formatter callback, or `false` to fall back to default popup text | Should return a String-compatible value |

**Callback Signature:** newFunction(x: double, y: double)

**Pitfalls:**
- Non-function values do not throw an error; the table silently falls back to default popup text (`x | y`).

**Cross References:**
- `$API.ScriptTable.setSnapValues$`
- `$API.ScriptTable.setMouseHandlingProperties$`

**Example:**
```javascript:script-table-popup-function
// Title: Use a custom popup formatter while dragging points
const var Table1 = Content.addTable("Table1", 0, 0);

inline function onTablePopupText(x, y)
{
    return "Pos " + x + " -> " + y;
}

Table1.setTablePopupFunction(onTablePopupText);
```

```json:testMetadata:script-table-popup-function
{
  "testable": false,
  "skipReason": "Popup callback is only invoked by interactive table dragging, which is not programmatically triggerable in snippet validation"
}
```

---

## setMouseHandlingProperties

**Signature:** `undefined setMouseHandlingProperties(JSON propertyObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setMouseHandlingProperties({"allowSwap": true, "snapWidth": 8.0});`

**Description:**
Configures table drag interaction behavior by forwarding a property object to the internal `TableEditor::MouseDragProperties` parser.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyObject | JSON | no | Mouse-drag configuration object | Uses known property keys listed below |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| syncStartEnd | bool | Mirror y value between first and last point while dragging edges |
| allowSwap | bool | Allow point crossing on the x axis |
| fixLeftEdge | double | Lock first point y value (`>-0.5` enables lock) |
| fixRightEdge | double | Lock last point y value (`>-0.5` enables lock) |
| snapWidth | double | Snap capture width in pixels |
| numSteps | int | Build evenly spaced snap grid (`-1` keeps current grid) |
| midPointSize | int | Midpoint handle size |
| dragPointSize | int | Regular drag point size |
| endPointSize | int | Edge point size |
| useMouseWheelForCurve | bool | Enable wheel-based curve editing |
| margin | double | Editor margin in pixels |
| closePath | bool | Close path shape for area fill |

**Cross References:**
- `$API.ScriptTable.setSnapValues$`
- `$API.ScriptTable.setTablePoint$`
- `$API.ScriptTable.setTablePopupFunction$`

**Example:**
```javascript:script-table-mouse-drag-properties
// Title: Configure snap, edge locks, and drag behavior
const var Table1 = Content.addTable("Table1", 0, 0);

Table1.setMouseHandlingProperties({
    "allowSwap": false,
    "syncStartEnd": true,
    "numSteps": 8,
    "snapWidth": 6.0,
    "fixLeftEdge": 0.0,
    "fixRightEdge": 1.0
});
```

```json:testMetadata:script-table-mouse-drag-properties
{
  "testable": false,
  "skipReason": "Behavior is user-interaction dependent (dragging and wheel edits) and cannot be deterministically triggered in snippet validation"
}
```

---

## setSnapValues

**Signature:** `undefined setSnapValues(Array snapValueArray)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setSnapValues([0.0, 0.25, 0.5, 0.75, 1.0]);`

**Description:**
Sets x-axis snap targets for drag operations.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| snapValueArray | Array | no | Normalized snap x positions | Values should be in 0.0-1.0 |

**Pitfalls:**
- [BUG] Passing a non-array reports an error, but the method still updates internal state before wrapper-side validation, which can make debugging mixed setups confusing.

**Cross References:**
- `$API.ScriptTable.setMouseHandlingProperties$`
- `$API.ScriptTable.setTablePopupFunction$`

---

## referToData

**Signature:** `undefined referToData(ScriptObject tableData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.referToData(otherTable);`

**Description:**
Rebinds this ScriptTable to another table data source. Accepts a `ScriptTableData` handle, another complex-data component with table data, or `-1` to return to its internal owned table.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableData | ScriptObject | no | External table source handle, compatible component, or reset token | `ScriptTableData`, compatible complex-data component, or `-1` |

**Pitfalls:**
- [BUG] Passing an unsupported argument type silently does nothing and keeps the previous data source.

**Cross References:**
- `$API.ScriptTable.registerAtParent$`

**Example:**
```javascript:script-table-refer-to-data
// Title: Share table data between two ScriptTable components
const var TableA = Content.addTable("TableA", 0, 0);
const var TableB = Content.addTable("TableB", 0, 80);

const var sharedData = TableA.registerAtParent(0);
TableB.referToData(sharedData);
```

```json:testMetadata:script-table-refer-to-data
{
  "testable": false,
  "skipReason": "Requires a ProcessorWithDynamicExternalData parent context to guarantee ScriptTableData registration in validator snippets"
}
```

---

## registerAtParent

**Signature:** `ScriptObject registerAtParent(Integer index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Minimal Example:** `var td = {obj}.registerAtParent(0);`

**Description:**
Registers this component's owned table at the parent processor's external-data slot and returns a `ScriptTableData` handle for that slot.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | External table slot index at the parent processor | >= 0 |

**Pitfalls:**
- [BUG] If the parent is not a `ProcessorWithDynamicExternalData`, this method returns `undefined` without a script error.

**Cross References:**
- `$API.ScriptTable.referToData$`

**Example:**
```javascript:script-table-register-at-parent
// Title: Register ScriptTable data and keep handle for external access
const var Table1 = Content.addTable("Table1", 0, 0);
const var tableData = Table1.registerAtParent(0);
```

```json:testMetadata:script-table-register-at-parent
{
  "testable": false,
  "skipReason": "Requires ProcessorWithDynamicExternalData host support that is not guaranteed in generic snippet validation"
}
```

---

## reset

**Signature:** `undefined reset()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.reset();`

**Description:**
Resets the table data to a default linear ramp from `(0, 0)` to `(1, 1)`.

**Cross References:**
- `$API.ScriptTable.addTablePoint$`
- `$API.ScriptTable.setTablePoint$`
