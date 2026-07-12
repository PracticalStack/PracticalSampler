## getValue

**Signature:** `double getValue()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var val = {obj}.getValue();`

**Description:**
Returns the current parameter value. If the parameter has an active DSP callback, returns the last value set through that callback. Otherwise falls back to the Value property stored in the parameter's ValueTree.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Parameter.setValue$`
- `$API.Parameter.setValueAsync$`
- `$API.Parameter.setValueSync$`

---

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the name of the parameter as defined in the node's parameter tree.

**Parameters:**
None.

**Pitfalls:**
None.

---

## getRangeObject

**Signature:** `var getRangeObject()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new DynamicObject on the heap.
**Minimal Example:** `var range = {obj}.getRangeObject();`

**Description:**
Returns the parameter's range properties as a JSON object with five properties: `MinValue` (range start), `MaxValue` (range end), `SkewFactor` (logarithmic skew), `StepSize` (discrete step interval), and `Inverted` (whether the range mapping is inverted). The returned object is a snapshot -- modifying it does not affect the parameter's range. Use `setRangeFromObject()` to apply changes.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Parameter.setRangeFromObject$`
- `$API.Parameter.setRangeProperty$`

---

## setRangeFromObject

**Signature:** `void setRangeFromObject(var propertyObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies ValueTree properties with undo support.
**Minimal Example:** `{obj}.setRangeFromObject({"MinValue": 0.0, "MaxValue": 100.0});`

**Description:**
Updates the parameter's range from a JSON object. Missing properties receive sensible defaults: MinValue = 0.0, MaxValue = 1.0, SkewFactor = 1.0, StepSize = 0.0, Inverted = false. Supports undo via the parent node's UndoManager.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyObject | JSON | no | Range configuration object with named properties | Must be a JSON object |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| MinValue | Double | Range minimum (default: 0.0) |
| MaxValue | Double | Range maximum (default: 1.0) |
| SkewFactor | Double | Logarithmic skew factor (default: 1.0) |
| StepSize | Double | Step interval for discrete values, 0 for continuous (default: 0.0) |
| Inverted | Integer | Whether the range mapping is inverted (default: false) |

**Pitfalls:**
None.

**Cross References:**
- `$API.Parameter.getRangeObject$`
- `$API.Parameter.setRangeProperty$`

**Example:**
```javascript:range-from-object
// Title: Configure parameter range with partial properties
const var nw = Synth.getExistingDspNetwork("myNetwork");
const var nd = nw.get("myOsc");
const var p = nd.getOrCreateParameter("Frequency");

// Only specify what differs from defaults
p.setRangeFromObject({
    "MinValue": 20.0,
    "MaxValue": 20000.0,
    "SkewFactor": 0.3
});

// Read back -- StepSize and Inverted have defaults
var range = p.getRangeObject();
Console.print(range.MinValue);   // 20
Console.print(range.StepSize);   // 0
Console.print(range.Inverted);   // 0
```
```json:testMetadata:range-from-object
{
  "testable": false,
  "skipReason": "Requires active scriptnode DspNetwork with nodes"
}
```

---

## setRangeProperty

**Signature:** `void setRangeProperty(String id, var newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies ValueTree property with notification chain.
**Minimal Example:** `{obj}.setRangeProperty({obj}.MinValue, 0.0);`

**Description:**
Sets a single range property by ID string. Valid property IDs are `MinValue`, `MaxValue`, `StepSize`, and `SkewFactor`. Use the Parameter constants (e.g. `p.MinValue`) for the first three. `SkewFactor` has no constant shortcut and must be passed as a string literal. Does not support undo, unlike `setRangeFromObject()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | Range property identifier | MinValue, MaxValue, StepSize, or SkewFactor |
| newValue | Number | no | New value for the property | Depends on the property |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "MinValue" | Sets the range minimum |
| "MaxValue" | Sets the range maximum |
| "StepSize" | Sets the discrete step interval (0 for continuous) |
| "SkewFactor" | Sets the logarithmic skew factor |

**Pitfalls:**
- [BUG] Invalid property ID strings are silently ignored. Passing `p.MidPoint` (which is a registered constant on Parameter) silently does nothing because `MidPoint` is not a recognized range property ID in the validation check. Only `MinValue`, `MaxValue`, `StepSize`, and `SkewFactor` are accepted.
- Does not support undo. Unlike `setRangeFromObject()`, changes made with `setRangeProperty()` are not recorded in the UndoManager.

**Cross References:**
- `$API.Parameter.setRangeFromObject$`
- `$API.Parameter.getRangeObject$`

---

## setValueAsync

**Signature:** `void setValueAsync(double newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValueAsync(0.5);`

**Deprecated:** Use `setValue()` after configuring the mode with `setUseExternalConnection()`. Marked deprecated via `DIAGNOSTIC_MARK_DEPRECATED`.

**Description:**
Sets the parameter value immediately by calling the DSP callback directly. The value is applied to all voices in polyphonic networks. Does not update the parameter's ValueTree and does not support undo. Use this for runtime automation and modulation where low latency is critical.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | Double | no | The new parameter value | Should be within the parameter's range |

**Pitfalls:**
- If the parameter's internal DSP callback is not yet initialized (the node has not been fully connected), the call is silently ignored without error.

**Cross References:**
- `$API.Parameter.setValue$`
- `$API.Parameter.setUseExternalConnection$`
- `$API.Parameter.setValueSync$`
- `$API.Parameter.getValue$`

---

## setValueSync

**Signature:** `void setValueSync(double newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies ValueTree property with undo manager; triggers synchronous listener chain.
**Minimal Example:** `{obj}.setValueSync(0.75);`

**Deprecated:** Use `setValue()` after configuring the mode with `setUseExternalConnection()`. Marked deprecated via `DIAGNOSTIC_MARK_DEPRECATED`.

**Description:**
Stores the value to the parameter's ValueTree with undo support. The ValueTree change triggers an internal listener that calls `setValueAsync()`, so the DSP callback is eventually invoked. Use this for UI-driven value changes and preset recall where undo support is needed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | Double | no | The new parameter value | Should be within the parameter's range |

**Pitfalls:**
None.

**Cross References:**
- `$API.Parameter.setValue$`
- `$API.Parameter.setUseExternalConnection$`
- `$API.Parameter.setValueAsync$`
- `$API.Parameter.getValue$`

---

## addConnectionFrom

**Signature:** `var addConnectionFrom(var connectionData)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Modifies ValueTree with undo, performs network traversal and node lookups.
**Minimal Example:** `var conn = {obj}.addConnectionFrom({"ID": "container1", "ParameterId": "Macro1"});`

**Description:**
Adds or removes a modulation connection to this parameter. Operates in two modes based on the argument type:

**Add mode** (object argument): Creates a connection from the source node and parameter described in the JSON object. Returns the new Connection object on success, or `undefined` if the connection cannot be created.

**Remove mode** (non-object argument): Pass any non-object value (e.g. `0` or `false`) to disconnect the existing connection. Clears the Automated flag and removes the connection from the ValueTree with undo support. Always returns `undefined`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| connectionData | NotUndefined | no | JSON connection descriptor (add) or any non-object value (remove) | Object for add mode, non-object for remove mode |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| ID | String | Source node ID in the network |
| ParameterId | String | Source parameter name on the source node |

**Pitfalls:**
- If the parameter already has a connection (Automated flag is true), passing a new connection descriptor silently returns `undefined` without creating the connection. Remove the existing connection first with `p.addConnectionFrom(0)`.
- Self-connections (where the source node and parameter match the target) are silently rejected with no error.
- If the source node ID does not match any node in the network, returns `undefined` without error.

**Cross References:**
- `$API.Connection.disconnect$`
- `$API.Node.connectTo$`

**Example:**
```javascript:add-remove-connection
// Title: Add and remove a parameter connection
const var nw = Synth.getExistingDspNetwork("myNetwork");
const var nd = nw.get("gain1");
const var p = nd.getOrCreateParameter("Gain");

// Add connection from a container macro parameter
var conn = p.addConnectionFrom({
    "ID": "container1",
    "ParameterId": "Macro1"
});

// Later: remove the connection
p.addConnectionFrom(0);
```
```json:testMetadata:add-remove-connection
{
  "testable": false,
  "skipReason": "Requires active scriptnode DspNetwork with connected nodes"
}
```

---

## setValue

**Signature:** `void setValue(double newValue)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Context-dependent. When `externalConnection` is true, dispatches to `setValueAsync()` (safe, direct DSP update). When false, dispatches to `setValueSync()` (unsafe, ValueTree with undo). In backend builds, throws a script error if called from the audio thread without external connection enabled.
**Minimal Example:** `{obj}.setValue(0.5);`

**Description:**
Unified value-setting method that dispatches to the appropriate path based on the parameter's connection mode. When `externalConnection` is enabled (via `setUseExternalConnection(true)`), calls `setValueAsync()` for immediate DSP update without undo. When disabled, calls `setValueSync()` for ValueTree-based update with undo support. This replaces the need to choose between `setValueAsync()` and `setValueSync()` manually -- configure the mode once with `setUseExternalConnection()`, then use `setValue()` throughout.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | Double | no | The new parameter value | Should be within the parameter's range |

**Pitfalls:**
- In backend builds, calling `setValue()` from the audio thread without first enabling external connection via `setUseExternalConnection(true)` throws a script error. In exported plugins this check is compiled out, and the call silently falls through to `setValueSync()` which performs unsafe ValueTree operations on the audio thread.

**Cross References:**
- `$API.Parameter.setUseExternalConnection$`
- `$API.Parameter.setValueAsync$`
- `$API.Parameter.setValueSync$`
- `$API.Parameter.getValue$`

---

## setUseExternalConnection

**Signature:** `void setUseExternalConnection(bool usesExternalConnection)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies CachedValue and ValueTree properties with undo manager.
**Minimal Example:** `{obj}.setUseExternalConnection(true);`

**Description:**
Configures whether `setValue()` dispatches to the async (direct DSP) or sync (ValueTree with undo) path. When set to `true`, removes the `Value` property from the parameter's ValueTree -- the parameter value lives only in the DSP callback and is not persisted. When set to `false`, restores the `Value` property using the current DSP value (if available) or the parameter's default value. Changes are recorded in the UndoManager. Call this once during setup, then use `setValue()` for all value changes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| usesExternalConnection | Integer | no | `true` for direct DSP path (audio-thread safe), `false` for ValueTree path (with undo) | Boolean value |

**Pitfalls:**
- When switching from external connection to non-external (`false`), the restored `Value` property uses the current DSP display value if the dynamic parameter is initialized, otherwise falls back to the `DefaultValue` property. If neither is meaningful, the parameter may reset to an unexpected value.

**Cross References:**
- `$API.Parameter.setValue$`
- `$API.Parameter.setValueAsync$`
- `$API.Parameter.setValueSync$`
