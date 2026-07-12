## canConnect

**Signature:** `bool canConnect(String source, String target)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Iterates ValueTree children internally.
**Minimal Example:** `var ok = {obj}.canConnect("LFO1", "CutoffMod");`

**Description:**
Checks whether a modulation connection between the given source and target can be made. Returns true if no connection between them currently exists, false if it already exists or if the source ID is not found in the source list.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| source | String | no | Source modulator ID | Must exist in getSourceList() |
| target | String | no | Target ID | Must exist in getTargetList() |

**Pitfalls:**
- Returns false for unknown source IDs without throwing an error, making it indistinguishable from "connection already exists."

**Cross References:**
- `$API.ScriptModulationMatrix.connect$`
- `$API.ScriptModulationMatrix.clearAllConnections$`
- `$API.ScriptModulationMatrix.getSourceList$`

## clearAllConnections

**Signature:** `void clearAllConnections(String targetId)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Uses killVoicesAndCall to suspend audio before modifying the connection tree.
**Minimal Example:** `{obj}.clearAllConnections("CutoffMod");`

**Description:**
Removes all modulation connections for the specified target. If an empty string is passed, removes all connections from the entire matrix. The operation suspends audio processing before modifying the connection tree.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| targetId | String | no | Target to clear connections for | Empty string clears all connections |

**Pitfalls:**
- Passing an empty string removes ALL connections from the matrix, not just connections with an empty target ID. This is the intended "clear everything" mechanism but can be surprising if an empty variable is passed accidentally.

**Cross References:**
- `$API.ScriptModulationMatrix.connect$`
- `$API.ScriptModulationMatrix.fromBase64$`

## connect

**Signature:** `bool connect(String sourceId, String targetId, bool addConnection)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Uses killVoicesAndCall to suspend audio before modifying the connection tree.
**Minimal Example:** `{obj}.connect("LFO1", "CutoffMod", true);`

**Description:**
Adds or removes a modulation connection between the specified source and target. When adding, the connection is initialized with default values based on the target type (see setMatrixModulationProperties for configuring defaults). When removing, the specific source-target connection is deleted. The operation suspends audio processing before modifying the connection tree.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceId | String | no | Source modulator ID | Must exist in getSourceList() |
| targetId | String | no | Target ID | Must exist in getTargetList() |
| addConnection | Integer | no | true to add, false to remove | -- |

**Pitfalls:**
- [BUG] Always returns false regardless of success or failure. The connection operation is deferred via killVoicesAndCall, and the internal result is discarded. Do not rely on the return value.
- [BUG] Silently does nothing if sourceId is not found in the source list. No error is thrown.

**Cross References:**
- `$API.ScriptModulationMatrix.canConnect$`
- `$API.ScriptModulationMatrix.clearAllConnections$`
- `$API.ScriptModulationMatrix.setConnectionProperty$`

## fromBase64

**Signature:** `void fromBase64(String b64)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Uses killVoicesAndCall to suspend audio before restoring the connection tree.
**Minimal Example:** `{obj}.fromBase64(savedState);`

**Description:**
Restores the modulation matrix state from a previously exported Base64 string. The string is decoded, decompressed (zstd), and the resulting ValueTree replaces all current connections. The operation suspends audio processing before modifying the connection tree. Silently does nothing if the Base64 string is invalid or decompression fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded matrix state from toBase64() | -- |

**Cross References:**
- `$API.ScriptModulationMatrix.toBase64$`
- `$API.ScriptModulationMatrix.clearAllConnections$`

## getComponent

**Signature:** `var getComponent(String targetId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Iterates all UI components with dynamic_cast checks.
**Minimal Example:** `var comp = {obj}.getComponent("CutoffMod");`

**Description:**
Returns the UI component (ScriptSlider or other ScriptComponent) associated with the given modulation target ID. For MatrixModulator targets, finds the component connected to the modulator's Value parameter. For parameter targets, finds the ScriptSlider with a matching matrixTargetId property. Returns undefined if no matching component is found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| targetId | String | no | Target ID to look up | Must match a target from getTargetList() |

**Cross References:**
- `$API.ScriptModulationMatrix.getTargetId$`
- `$API.ScriptModulationMatrix.getTargetList$`

## getConnectionProperty

**Signature:** `var getConnectionProperty(String sourceId, String targetId, String propertyId)`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Call Scope Note:** ValueTree property access with internal locking.
**Minimal Example:** `var intensity = {obj}.getConnectionProperty("LFO1", "CutoffMod", "Intensity");`

**Description:**
Returns the value of a specific property on the connection between the given source and target. Returns undefined if the source is not found, if no connection exists between the source and target, or if the property ID is not a valid watchable property.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceId | String | no | Source modulator ID | Must exist in getSourceList() |
| targetId | String | no | Target ID | Must exist in getTargetList() |
| propertyId | String | no | Connection property name | One of the valid property IDs |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "SourceIndex" | Index into the source list (-1 = disconnected) |
| "TargetId" | The target identifier string |
| "Intensity" | Modulation depth, range [-1.0, 1.0] |
| "Mode" | Modulation mode: 0 = Scale, 1 = Unipolar, 2 = Bipolar |
| "Inverted" | Whether the modulation signal is inverted (boolean) |
| "AuxIndex" | Secondary source index (-1 = none) |
| "AuxIntensity" | Secondary source modulation depth |

**Pitfalls:**
- Returns undefined for all three failure cases (invalid source, no connection, invalid property) with no way to distinguish between them.

**Cross References:**
- `$API.ScriptModulationMatrix.setConnectionProperty$`
- `$API.ScriptModulationMatrix.connect$`

## getMatrixModulationProperties

**Signature:** `var getMatrixModulationProperties()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new JSON object via the container's toJSON method.
**Minimal Example:** `var props = {obj}.getMatrixModulationProperties();`

**Description:**
Returns a JSON object containing the current global matrix modulation properties, including selectable sources mode, default init values, and range properties. This is the inverse of setMatrixModulationProperties.

**Parameters:**

(none)

**Cross References:**
- `$API.ScriptModulationMatrix.setMatrixModulationProperties$`

## getModulationDisplayData

**Signature:** `var getModulationDisplayData(String targetId)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Accesses slider state, creates DynamicObject, may cache query functions.
**Minimal Example:** `var data = {obj}.getModulationDisplayData("CutoffMod");`

**Description:**
Returns a JSON object containing modulation visualization data for the specified target. The object contains normalized values, scaled values, modulation ranges, and active state for drawing custom modulation displays. Results are cached after the first call per target. Returns undefined if no matching target is found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| targetId | String | no | Target ID to query | Must match a target from getTargetList() |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| valueNormalized | double | The slider's current normalized value (0-1) |
| scaledValue | double | Value after scale-mode modulation is applied |
| addValue | double | Cumulative additive modulation offset |
| modulationActive | bool | Whether any modulation connections are active |
| modMinValue | double | Lower bound of the modulation range |
| modMaxValue | double | Upper bound of the modulation range |
| lastModValue | double | Previous modulation output value |

**Cross References:**
- `$API.ScriptModulationMatrix.getComponent$`
- `$API.ScriptModulationMatrix.getTargetList$`
- `$API.ScriptModulationMatrix.setConnectionProperty$`

## getSourceList

**Signature:** `var getSourceList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new Array from the internal source list.
**Minimal Example:** `var sources = {obj}.getSourceList();`

**Description:**
Returns an array of strings containing the IDs of all modulation sources. Sources are the modulator processors in the GlobalModulatorContainer's gain modulation chain, identified by their processor ID.

**Parameters:**

(none)

**Cross References:**
- `$API.ScriptModulationMatrix.getTargetList$`
- `$API.ScriptModulationMatrix.canConnect$`

## getTargetId

**Signature:** `String getTargetId(var componentOrId)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Looks up components by name, involves string construction and dynamic_cast.
**Minimal Example:** `var id = {obj}.getTargetId(mySlider);`

**Description:**
Returns the modulation target ID for the given UI component. Accepts either a direct component reference or a component name string. For components connected to a MatrixModulator's Value parameter, returns the modulator's processor ID. For ScriptSliders with a matrixTargetId property, returns that property value. Returns an empty string if the component is not found or has no target ID.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentOrId | NotUndefined | no | Component reference or component name string | -- |

**Cross References:**
- `$API.ScriptModulationMatrix.getComponent$`
- `$API.ScriptModulationMatrix.getTargetList$`

## getTargetList

**Signature:** `var getTargetList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new Array from the internal target list.
**Minimal Example:** `var targets = {obj}.getTargetList();`

**Description:**
Returns an array of strings containing the IDs of all modulation targets. Targets come from two sources: MatrixModulator processors in the module tree (identified by their processor ID or custom target ID) and ScriptSlider components with a non-empty matrixTargetId property.

**Parameters:**

(none)

**Cross References:**
- `$API.ScriptModulationMatrix.getSourceList$`
- `$API.ScriptModulationMatrix.getComponent$`

## setConnectionCallback

**Signature:** `void setConnectionCallback(Function updateFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a WeakCallbackHolder.
**Minimal Example:** `{obj}.setConnectionCallback(onMatrixChanged);`

**Description:**
Registers a callback that fires whenever a modulation connection is added or removed from the matrix. The callback receives the source name, target ID, and a boolean indicating whether the connection was added (true) or removed (false). The callback fires synchronously when the connection tree changes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| updateFunction | Function | yes | Callback for connection changes | Must accept 3 arguments |

**Callback Signature:** updateFunction(sourceId: String, targetId: String, wasAdded: bool)

**Example:**
```javascript:connection-change-tracking
// Title: Connection change tracking
const var mm = Engine.createModulationMatrix("Global Modulator Container0");

inline function onMatrixChanged(sourceId, targetId, wasAdded)
{
    Console.print(sourceId + " -> " + targetId + (wasAdded ? " connected" : " disconnected"));
};

mm.setConnectionCallback(onMatrixChanged);
```

```json:testMetadata:connection-change-tracking
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with modulators and targets in the module tree"
}
```

**Cross References:**
- `$API.ScriptModulationMatrix.connect$`
- `$API.ScriptModulationMatrix.clearAllConnections$`
- `$API.ScriptModulationMatrix.setDragCallback$`

## setConnectionProperty

**Signature:** `bool setConnectionProperty(String sourceId, String targetId, String propertyId, var value)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** ValueTree setProperty with undo manager, triggers property listeners.
**Minimal Example:** `{obj}.setConnectionProperty("LFO1", "CutoffMod", "Intensity", 0.75);`

**Description:**
Sets the value of a specific property on the connection between the given source and target. The change is recorded in the undo manager for undo/redo support. Returns true if the property was successfully set, false if the source was not found, no connection exists, or the property ID is invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceId | String | no | Source modulator ID | Must exist in getSourceList() |
| targetId | String | no | Target ID | Must exist in getTargetList() |
| propertyId | String | no | Connection property name | See getConnectionProperty for valid values |
| value | NotUndefined | no | New property value | Type depends on property |

**Cross References:**
- `$API.ScriptModulationMatrix.getConnectionProperty$`
- `$API.ScriptModulationMatrix.connect$`

## setCurrentlySelectedSource

**Signature:** `void setCurrentlySelectedSource(String sourceId)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls setExlusiveMatrixSource which modifies container state.
**Minimal Example:** `{obj}.setCurrentlySelectedSource("LFO1");`

**Description:**
Sets the currently selected modulation source in exclusive source selection mode. Fires the source selection callback if one is registered. Throws a script error if selectable sources are not enabled (via setMatrixModulationProperties or setSourceSelectionCallback).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceId | String | no | Source modulator ID to select | Must exist in getSourceList() |

**Pitfalls:**
- Calling this without first enabling selectable sources throws a script error. Selectable sources can be enabled either by calling setMatrixModulationProperties with SelectableSources: true, or by registering a source selection callback via setSourceSelectionCallback.

**Cross References:**
- `$API.ScriptModulationMatrix.setSourceSelectionCallback$`
- `$API.ScriptModulationMatrix.setMatrixModulationProperties$`

## setDragCallback

**Signature:** `void setDragCallback(Function newDragCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a WeakCallbackHolder and registers a broadcaster listener.
**Minimal Example:** `{obj}.setDragCallback(onDrag);`

**Description:**
Registers a callback that fires during drag-and-drop interaction with modulation connections. The callback receives the source ID (empty string if no source), the target ID, and an action string. Any previously registered drag callback is removed before the new one is set.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newDragCallback | Function | yes | Callback for drag events | Must accept 3 arguments |

**Callback Signature:** newDragCallback(sourceId: String, targetId: String, action: String)

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "DragStart" | A modulation source drag operation has begun |
| "DragEnd" | The drag operation was cancelled or completed |
| "Drop" | The source was dropped on a valid target, creating a connection |
| "Hover" | The dragged source is hovering over a valid target |
| "DisabledHover" | The dragged source is hovering over an invalid or occupied target |

**Example:**
```javascript:drag-interaction-feedback
// Title: Drag interaction feedback
const var mm = Engine.createModulationMatrix("Global Modulator Container0");

inline function onDrag(sourceId, targetId, action)
{
    if (action == "Hover")
        Console.print("Can connect " + sourceId + " to " + targetId);

    if (action == "Drop")
        Console.print("Connected " + sourceId + " to " + targetId);
};

mm.setDragCallback(onDrag);
```

```json:testMetadata:drag-interaction-feedback
{
  "testable": false,
  "skipReason": "Requires drag interaction with the modulation matrix UI"
}
```

**Cross References:**
- `$API.ScriptModulationMatrix.connect$`
- `$API.ScriptModulationMatrix.setConnectionCallback$`

## setEditCallback

**Signature:** `void setEditCallback(ComplexType menuItems, Function editFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a WeakCallbackHolder, modifies container state, registers broadcaster listener.
**Minimal Example:** `{obj}.setEditCallback(["Edit", "Remove"], onEdit);`

**Description:**
Registers custom context menu items and a callback for the "Edit connections" action on modulation targets. The menuItems parameter defines the popup menu entries (as an array of strings or a single string). The callback fires when a menu item is selected, receiving the zero-based menu index and the target ID. Passing an invalid function or empty menu items clears the edit callback and removes any custom menu entries.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| menuItems | ComplexType | yes | Menu item labels | Array of Strings or single String |
| editFunction | Function | yes | Callback for menu selection | Must accept 2 arguments |

**Callback Signature:** editFunction(menuIndex: int, targetId: String)

**Pitfalls:**
- Empty strings and duplicates are automatically removed from the menu items array. If all items are empty or duplicates that collapse to nothing, the callback is cleared instead of registered.

**Example:**
```javascript:custom-edit-menu
// Title: Custom edit menu for modulation targets
const var mm = Engine.createModulationMatrix("Global Modulator Container0");

inline function onEdit(menuIndex, targetId)
{
    if (menuIndex == 0)
        Console.print("Editing connections for " + targetId);

    if (menuIndex == 1)
        mm.clearAllConnections(targetId);
};

mm.setEditCallback(["Edit Ranges", "Clear All"], onEdit);
```

```json:testMetadata:custom-edit-menu
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer and user interaction with the edit menu"
}
```

**Cross References:**
- `$API.ScriptModulationMatrix.clearAllConnections$`
- `$API.ScriptModulationMatrix.setConnectionCallback$`

## setMatrixModulationProperties

**Signature:** `void setMatrixModulationProperties(var newProperties)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Parses JSON and modifies container state.
**Minimal Example:** `{obj}.setMatrixModulationProperties({"SelectableSources": true});`

**Description:**
Configures global properties for the modulation matrix system. The JSON object can contain three sections: SelectableSources (boolean flag for exclusive source selection mode), DefaultInitValues (per-target default connection values applied when new connections are created), and RangeProperties (per-target range and display configuration). Throws a script error if a DefaultInitValues entry has a non-zero Intensity but no Mode property.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newProperties | JSON | no | Global matrix properties | See Callback Properties for schema |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| SelectableSources | bool | Enable exclusive source selection mode |
| DefaultInitValues | Object | Per-target map of default connection values |
| RangeProperties | Object | Per-target range configuration (preset name or full object) |

**Example:**
```javascript:matrix-properties-config
// Title: Configure matrix properties with defaults and ranges
const var mm = Engine.createModulationMatrix("Global Modulator Container0");

mm.setMatrixModulationProperties({
    "SelectableSources": false,
    "DefaultInitValues": {
        "CutoffMod": {
            "Intensity": 0.5,
            "IsNormalized": true,
            "Mode": "Unipolar"
        }
    },
    "RangeProperties": {
        "CutoffMod": "FilterFreq",
        "GainMod": {
            "InputRange": {"MinValue": 0.0, "MaxValue": 1.0},
            "OutputRange": {"MinValue": 0.0, "MaxValue": 1.0},
            "mode": "NormalizedPercentage"
        }
    }
});
```

```json:testMetadata:matrix-properties-config
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with matching target IDs in the module tree"
}
```

**Cross References:**
- `$API.ScriptModulationMatrix.getMatrixModulationProperties$`
- `$API.ScriptModulationMatrix.connect$`

## setSourceSelectionCallback

**Signature:** `void setSourceSelectionCallback(Function newCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a WeakCallbackHolder, modifies container state, registers broadcaster listener.
**Minimal Example:** `{obj}.setSourceSelectionCallback(onSourceSelected);`

**Description:**
Registers a callback that fires whenever a new modulation source is selected in exclusive source mode. As a side effect, registering a callback automatically enables selectable sources mode on the container. Passing a non-function value disables selectable sources mode and removes the listener. Any previously registered source selection callback is removed before the new one is set.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newCallback | Function | yes | Callback for source selection | Must accept 1 argument |

**Callback Signature:** newCallback(sourceName: String)

**Pitfalls:**
- Registering this callback automatically enables selectable sources mode as a side effect. Passing a non-function value (to clear the callback) automatically disables selectable sources mode. This implicit state change is not obvious from the method name.

**Example:**
```javascript:source-selection-tracking
// Title: Source selection tracking
const var mm = Engine.createModulationMatrix("Global Modulator Container0");

inline function onSourceSelected(sourceName)
{
    Console.print("Selected source: " + sourceName);
};

mm.setSourceSelectionCallback(onSourceSelected);
```

```json:testMetadata:source-selection-tracking
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with modulators in the module tree"
}
```

**Cross References:**
- `$API.ScriptModulationMatrix.setCurrentlySelectedSource$`
- `$API.ScriptModulationMatrix.setMatrixModulationProperties$`
- `$API.ScriptModulationMatrix.setConnectionCallback$`

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Compresses ValueTree data and performs Base64 encoding (heap allocations).
**Minimal Example:** `var state = {obj}.toBase64();`

**Description:**
Creates a Base64-encoded string representing the current state of all modulation connections. The ValueTree connection data is compressed using zstd before encoding. The resulting string can be stored and later restored with fromBase64.

**Parameters:**

(none)

**Cross References:**
- `$API.ScriptModulationMatrix.fromBase64$`
