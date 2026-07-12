# Node -- Method Entries

## get

**Signature:** `var get(var id)`
**Return Type:** `var`
**Call Scope:** warning
**Call Scope Note:** String involvement -- id.toString() and Identifier construction involve atomic ref-count operations.
**Minimal Example:** `var mode = {obj}.get("Mode");`

**Description:**
Returns the value of a node-type-specific property from the node's Properties child tree. Only reads node properties (e.g. "Mode", "Frequency") -- does not read direct ValueTree properties like Bypassed or NodeColour. Returns undefined if the property ID is not found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | Property ID to read | Must match a node-type-specific property ID |

**Pitfalls:**
- Only reads from the node Properties child tree. Direct ValueTree properties (Bypassed, NodeColour, Comment, Folded) are not accessible via `get()` even though `set()` can write to them. Use `isBypassed()` for bypass state.

**Cross References:**
- `$API.Node.set$`
- `$API.Node.isBypassed$`

## set

**Signature:** `void set(var id, var value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Writes ValueTree properties via undo manager, involving heap allocations and notification chains.
**Minimal Example:** `{obj}.set("Mode", 1);`

**Description:**
Sets a property value on the node. Checks two property locations independently: if the ID matches a node-type-specific property (in the Properties child tree), it writes there; if the ID matches a direct ValueTree property (Bypassed, NodeColour, Comment, Folded), it also writes there. If the ID is not found in either location, the call silently does nothing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | Property ID to write | Node property or ValueTree property ID |
| value | var | no | New value for the property | Type depends on the property |

**Pitfalls:**
- Silently does nothing if the property ID does not exist in either location. No error is reported for unknown property names.
- `get()` only reads node properties, but `set()` writes to both node properties and ValueTree properties. Setting a ValueTree property like "NodeColour" via `set()` succeeds, but `get("NodeColour")` returns undefined because `get()` only checks the node Properties child tree.

**Cross References:**
- `$API.Node.get$`
- `$API.Node.setBypassed$`

## isBypassed

**Signature:** `bool isBypassed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bypassed = {obj}.isBypassed();`

**Description:**
Returns whether the node is currently bypassed. Reads from the internal bypass state member, not directly from the ValueTree property.

**Parameters:**
None.

**Cross References:**
- `$API.Node.setBypassed$`

## setBypassed

**Signature:** `void setBypassed(bool shouldBeBypassed)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a simple boolean member variable with no heap allocation or locking.
**Minimal Example:** `{obj}.setBypassed(true);`

**Description:**
Sets the node's bypass state. This is a virtual method -- the actual bypass behavior (silence output, pass-through signal) is implemented by each node type's processing overrides. Sets the internal bypass state member; the ValueTree Bypassed property is synchronized separately via a property listener.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Integer | no | Whether to bypass the node | true to bypass, false to enable |

**Cross References:**
- `$API.Node.isBypassed$`
- `$API.Node.connectToBypass$`

## reset

**Signature:** `void reset()`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Pure virtual method designed for audio-thread invocation. Concrete implementations must be lock-free.
**Minimal Example:** `{obj}.reset();`

**Description:**
Resets the node's internal DSP state (clears buffers, resets filters, etc.). This is a pure virtual method -- each concrete node type implements its own reset logic. Called automatically at voice start in polyphonic contexts.

**Parameters:**
None.

## getNumParameters

**Signature:** `int getNumParameters()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.getNumParameters();`

**Description:**
Returns the number of parameters on this node. For leaf nodes, this is the fixed parameter count defined by the node's factory type. For container nodes, this includes any dynamically created macro parameters.

**Parameters:**
None.

**Cross References:**
- `$API.Node.getOrCreateParameter$`

## getChildNodes

**Signature:** `var getChildNodes(bool recursive)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new Array and iterates children with heap allocations. Recursive mode traverses the full subtree.
**Minimal Example:** `var children = {obj}.getChildNodes(false);`

**Description:**
Returns an array of child Node references. Only container nodes (container.chain, container.split, etc.) have children -- leaf nodes return an empty array. When recursive is true, the method traverses the full subtree depth-first, returning all descendant nodes in a flat array.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| recursive | Integer | no | Whether to include descendants of child containers | true for full subtree, false for direct children only |

**Cross References:**
- `$API.Node.setParent$`

## getOrCreateParameter

**Signature:** `var getOrCreateParameter(var indexOrId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** When creating: modifies ValueTree structure with undo manager. When retrieving: performs tree search with string comparison.
**Minimal Example:** `var p = {obj}.getOrCreateParameter("Volume");`

**Description:**
Retrieves an existing parameter by name, index, or JSON descriptor, or creates a new one if not found. Parameter creation is only supported on container nodes (container.chain, container.split, etc.) -- calling on a leaf node produces a script error.

The method first tries exact parameter lookup: by name (String), integer index, or JSON object with an ID property. If found, the existing Parameter object is returned. If not found and the node is a container, a new parameter is created from the JSON descriptor's range and mode properties.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| indexOrId | Object | no | Parameter name (String), index (Integer), or JSON object with ID and range properties | See Callback Properties for JSON creation format |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| ID | String | Parameter name identifier (required for creation) |
| MinValue | Double | Minimum parameter value |
| MaxValue | Double | Maximum parameter value |
| SkewFactor | Double | Skew factor for non-linear value distribution |
| StepSize | Double | Step size for discrete value snapping |
| mode | String | Text-to-value converter mode (e.g. "Frequency", "Decibel") |

**Cross References:**
- `$API.Node.getNumParameters$`
- `$API.Node.connectTo$`

**Example:**
```javascript:create-macro-parameter
// Title: Create a macro parameter on a container node
const var nw = Engine.createDspNetwork("MyNetwork");
const var chain = nw.get("myChain");

var p = chain.getOrCreateParameter({
    "ID": "Volume",
    "MinValue": -100.0,
    "MaxValue": 0.0,
    "SkewFactor": 5.0,
    "StepSize": 0.1,
    "mode": "Decibel"
});
```

```json:testMetadata:create-macro-parameter
{
  "testable": false,
  "skipReason": "Requires a DspNetwork::Holder processor (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator)"
}
```

## setParent

**Signature:** `void setParent(var parentNode, int indexInParent)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies ValueTree structure with undo manager. Removes from current parent and adds to new parent.
**Minimal Example:** `{obj}.setParent(chain, 0);`

**Description:**
Moves this node to a new parent container at the specified index position. The parent must be a container node (container.chain, container.split, etc.). Passing the DspNetwork itself as the parent auto-converts to the network's root node. Passing an empty or null parent detaches the node from the graph. Existing parameter connections are preserved during the move via an internal automation preserver.

The node is removed from its current parent before the target is validated. If the target lookup fails, a script error is reported for non-empty parent names, but the node has already been detached.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parentNode | ScriptObject | no | Target container Node, DspNetwork reference, or empty to detach | Must be a container node or DspNetwork |
| indexInParent | Integer | no | Position index within the target's child list | -1 to append at end |

**Pitfalls:**
- [BUG] The node is removed from its current parent before the target container is validated. If the target is not found or is not a container, the node has already been detached from the graph. In backend builds with undo enabled, this can be reversed.

**Cross References:**
- `$API.Node.getChildNodes$`
- `$API.DspNetwork.get$`

## connectTo

**Signature:** `var connectTo(var parameterTarget, var sourceInfo)`
**Return Type:** `var`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to virtual addModulationConnection which modifies ValueTree structure.
**Minimal Example:** `{obj}.connectTo(targetParam, "MacroParam");`

**Description:**
Creates a connection from this node to a target parameter. The behavior depends on this node's type:

- **Container nodes** (container.chain, container.split): Creates a macro parameter connection. sourceInfo is the name (String) of the source macro parameter on this container.
- **Modulation source nodes**: Creates a modulation target connection. sourceInfo is the output slot index (Integer).
- **Other node types**: The base implementation returns undefined silently.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterTarget | ScriptObject | no | Target Parameter object obtained from getOrCreateParameter | Must be a Node Parameter reference |
| sourceInfo | var | no | Source identifier -- parameter name (String) for containers, output slot index (Integer) for modulation sources | Depends on the source node type |

**Pitfalls:**
- On leaf nodes that are not modulation sources, silently returns undefined without creating any connection or reporting an error.
- If parameterTarget is not a valid Parameter object (wrong type or undefined), silently returns undefined.

**Cross References:**
- `$API.Node.getOrCreateParameter$`
- `$API.Node.connectToBypass$`

## connectToBypass

**Signature:** `void connectToBypass(var sourceInfo)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies ValueTree connection structure with undo manager.
**Minimal Example:** `{obj}.connectToBypass(dragDetails);`

**Description:**
Creates or removes a dynamic bypass connection on this node. The method operates in two modes:

**Connect mode:** When sourceInfo contains valid source parameter or modulation source data (an internal drag details object), a new Connection entry is created in the source's connection tree, targeting this node's Bypassed property.

**Disconnect mode:** When no valid source is found in sourceInfo, the method searches for and removes any existing bypass connection targeting this node. It checks container macro parameter connections, switch target connections, and modulation source trees.

Dynamic bypass connections require the target node to be a SoftBypassNode. The validation error (IllegalBypassConnection) is raised when the connection is processed downstream, not in this method.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceInfo | Object | no | Drag details object with source parameter or modulation source info, or empty to disconnect | Internal drag details format |

**Cross References:**
- `$API.Node.setBypassed$`
- `$API.Node.connectTo$`

## setComplexDataIndex

**Signature:** `bool setComplexDataIndex(String dataType, int dataSlot, int indexValue)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Sets ValueTree property via undo manager.
**Minimal Example:** `{obj}.setComplexDataIndex("Table", 0, 2);`

**Description:**
Changes which external data slot a node references for a specific data type. The node must have a ComplexData child tree in its ValueTree (i.e., it must be a data-consuming node). Returns true if the index was set successfully, false if the node has no ComplexData tree, the data type string is not recognized, or the slot index is out of range.

Internally, the method appends "s" to the dataType string to find the correct child tree (e.g. "Table" becomes "Tables"), then locates the child at the specified slot index and sets its Index property.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataType | String | no | The complex data type name in singular form | See Value Descriptions |
| dataSlot | Integer | no | The data slot index on the node | 0-based, must be a valid slot |
| indexValue | Integer | no | The new data index to reference in the pool | 0-based |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Table" | Lookup table curve data (512 floats) |
| "SliderPack" | Resizable float array displayed as vertical sliders |
| "AudioFile" | Multichannel audio file reference |
| "FilterCoefficients" | Filter coefficient display data |
| "DisplayBuffer" | FIFO visualization buffer for oscilloscope or spectrum display |

**Cross References:**
- `$API.DspNetwork.create$`

## getIndexInParent

**Disabled:** no-op
**Disabled Reason:** Not registered via ADD_API_METHOD in the constructor. The C++ method exists with a doc comment but has no scripting wrapper, making it uncallable from HISEScript.

## isActive

**Disabled:** no-op
**Disabled Reason:** Not registered via ADD_API_METHOD in the constructor. The C++ method exists with a doc comment but has no scripting wrapper, making it uncallable from HISEScript.

## getNodeHolder

**Disabled:** no-op
**Disabled Reason:** Not registered via ADD_API_METHOD in the constructor. The C++ method is defined outside the NODE API section and has no scripting wrapper, making it uncallable from HISEScript.
