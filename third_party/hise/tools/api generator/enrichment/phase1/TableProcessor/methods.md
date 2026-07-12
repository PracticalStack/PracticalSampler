# TableProcessor -- Method Analysis

## addTablePoint

**Signature:** `void addTablePoint(int tableIndex, float x, float y)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addTablePoint(0, 0.5, 0.75);`

**Description:**
Adds a new graph point to the table at the specified position. The point is appended to the internal graph point array with a default curve of 0.5 (linear interpolation). After adding, the lookup table is recalculated and a UI update notification is sent.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableIndex | Integer | no | Index of the table to modify. Most modules have one table (index 0). | >= 0 |
| x | Double | no | Horizontal position of the new point (normalized). | 0.0-1.0 recommended |
| y | Double | no | Vertical position of the new point (normalized). | 0.0-1.0 recommended |

**Pitfalls:**
- The curve value is always set to 0.5 (linear interpolation). To set a custom curve on the added point, call `setTablePoint` afterward with the desired curve value.
- [BUG] Unlike `setTablePoint`, this method does not clamp x and y to the 0.0-1.0 range. Out-of-range values are stored as-is and may produce unexpected table shapes.

**Cross References:**
- `$API.TableProcessor.setTablePoint$`
- `$API.TableProcessor.getTable$`
- `$API.TableProcessor.reset$`

## exists

**Signature:** `bool exists()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.exists();`

**Description:**
Returns whether the underlying processor reference is still valid. Returns false if the processor has been deleted or if the TableProcessor was constructed with a null reference (e.g., when `Synth.getTableProcessor` could not find the specified module).

**Parameters:**

None.

## exportAsBase64

**Signature:** `String exportAsBase64(int tableIndex)`
**Return Type:** `String`
**Call Scope:** unsafe
**Minimal Example:** `var state = {obj}.exportAsBase64(0);`

**Description:**
Serializes the table's graph points to a base64-encoded string. The string captures all point positions and curve values and can be restored later with `restoreFromBase64`. Returns an empty string when the table is in its default state (a diagonal line from (0,0) to (1,1) with 0.5 curve).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableIndex | Integer | no | Index of the table to export. Most modules have one table (index 0). | >= 0 |

**Pitfalls:**
- Returns an empty string for the default table state rather than a base64 representation. This is an optimization -- `restoreFromBase64` handles empty strings by calling `reset()`, so the serialization roundtrip is consistent.

**Cross References:**
- `$API.TableProcessor.restoreFromBase64$`
- `$API.TableProcessor.reset$`

## getTable

**Signature:** `var getTable(int tableIndex)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Minimal Example:** `var table = {obj}.getTable(0);`

**Description:**
Returns a `Table` data object for the specified table index. The `Table` object wraps the same underlying table data but provides a richer API surface including `getTableValueNormalised()` for evaluating the table curve at any position, `setDisplayCallback()` for ruler change notifications, and direct point editing methods without the `tableIndex` parameter. Changes made through the `Table` object are reflected in the parent module and vice versa.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableIndex | Integer | no | Index of the table to retrieve. Most modules have one table (index 0). | >= 0 |

**Cross References:**
- `$API.TableProcessor.addTablePoint$`
- `$API.TableProcessor.setTablePoint$`
- `$API.TableProcessor.reset$`
- `$API.TableProcessor.exportAsBase64$`
- `$API.TableProcessor.restoreFromBase64$`

**Example:**
```javascript:get-table-object
// Title: Extracting a Table data object for direct evaluation
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
var ss = builder.create(builder.SoundGenerators.SineSynth, "TestSynth", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.Velocity, "VelocityMod", ss, builder.ChainIndexes.Gain);
builder.flush();
// --- end setup ---

const var tp = Synth.getTableProcessor("VelocityMod");
const var table = tp.getTable(0);

// Add a midpoint and read back the interpolated curve
table.addTablePoint(0.5, 0.8);
var curveValue = table.getTableValueNormalised(0.25);
Console.print("Interpolated value at 0.25: " + curveValue);
```

```json:testMetadata:get-table-object
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Math.abs(table.getTableValueNormalised(0.5) - 0.8) < 0.01", "value": true}
  ]
}
```

## reset

**Signature:** `void reset(int tableIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.reset(0);`

**Description:**
Resets the table to its default state: a straight diagonal line from (0, 0) to (1, 1) with a curve of 0.5. All existing graph points are removed and replaced with these two default endpoints. The lookup table is recalculated and a UI update notification is sent.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableIndex | Integer | no | Index of the table to reset. Most modules have one table (index 0). | >= 0 |

**Cross References:**
- `$API.TableProcessor.addTablePoint$`
- `$API.TableProcessor.restoreFromBase64$`

## restoreFromBase64

**Signature:** `void restoreFromBase64(int tableIndex, String state)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.restoreFromBase64(0, savedState);`

**Description:**
Restores the table's graph points from a base64-encoded string previously created by `exportAsBase64`. If the state string is empty, the table is reset to its default state (equivalent to calling `reset()`). If the string contains invalid base64 data that decodes to zero bytes, the method returns without modifying the table.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableIndex | Integer | no | Index of the table to restore. Most modules have one table (index 0). | >= 0 |
| state | String | no | Base64-encoded table state from `exportAsBase64`. | Valid base64 or empty string |

**Cross References:**
- `$API.TableProcessor.exportAsBase64$`
- `$API.TableProcessor.reset$`
- `$API.TableProcessor.getTable$`

## setTablePoint

**Signature:** `void setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTablePoint(0, 1, 0.5, 0.8, 0.3);`

**Description:**
Modifies an existing graph point in the table. All coordinate and curve values are clamped to the 0.0-1.0 range internally. For non-edge points (not the first or last), all three properties (x, y, curve) are updated. For edge points (first and last), only y and curve are updated -- the x position is preserved at its fixed endpoint.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableIndex | Integer | no | Index of the table to modify. Most modules have one table (index 0). | >= 0 |
| pointIndex | Integer | no | Index of the graph point to modify. | 0 to numPoints-1 |
| x | Double | no | New horizontal position (normalized, clamped to 0.0-1.0). Ignored for edge points. | 0.0-1.0 |
| y | Double | no | New vertical position (normalized, clamped to 0.0-1.0). | 0.0-1.0 |
| curve | Double | no | Interpolation curve value (clamped to 0.0-1.0). 0.5 is linear. | 0.0-1.0 |

**Pitfalls:**
- Edge points (index 0 and the last point) silently ignore the x parameter. Only y and curve are updated. No error or warning is produced when attempting to set x on an edge point.
- [BUG] An out-of-range pointIndex is silently ignored -- no error is reported and no modification occurs. The call appears to succeed but has no effect.

**Cross References:**
- `$API.TableProcessor.addTablePoint$`
- `$API.TableProcessor.getTable$`
- `$API.TableProcessor.reset$`
