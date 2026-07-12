# DspNetwork -- Method Analysis

## clear

**Signature:** `undefined clear(Integer removeNodesFromSignalChain, Integer removeUnusedNodes)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Removes ValueTree children with undo manager notifications. When removeUnusedNodes is true, acquires MessageManagerLock inside the cleanup loop.
**Minimal Example:** `{obj}.clear(true, true);`

**Description:**
Removes nodes from the network. When `removeNodesFromSignalChain` is true, removes all child nodes and parameters from the root container's ValueTree. When `removeUnusedNodes` is true, iterates all registered nodes and deletes those not currently in any signal path, then triggers garbage collection of filter coefficient objects on the external data holder.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| removeNodesFromSignalChain | Integer | no | If true, removes all child nodes and parameters from the root node | Boolean: 0 or 1 |
| removeUnusedNodes | Integer | no | If true, deletes nodes not in any signal path | Boolean: 0 or 1 |

**Pitfalls:**
- The two flags are independent: `clear(true, false)` detaches nodes from the root container but leaves them registered in the network's node list as orphaned objects. A subsequent `clear(false, true)` is needed to remove them entirely. Passing `clear(false, true)` only removes nodes that are already orphaned without touching the signal chain.

**Cross References:**
- `$API.DspNetwork.create$`
- `$API.DspNetwork.deleteIfUnused$`

## create

**Signature:** `ScriptObject create(String path, String id)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a ValueTree and instantiates a node object via factory lookup. Involves heap allocations and string operations.
**Minimal Example:** `var node = {obj}.create("core.gain", "myGain");`

**Description:**
Creates a node with the given factory path and ID and returns a reference to it. If a node with the specified `id` already exists in the network, returns the existing node without creating a new one. If `id` is empty, a unique name is auto-generated from the path suffix. The factory path uses `factory.node` format (e.g. `core.gain`, `container.split`, `math.add`). Returns undefined if no registered factory matches the path.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | String | no | Factory path in `factory.node` format | Must match a registered node factory |
| id | String | no | Unique node identifier | If empty, auto-generated from path suffix |

**Pitfalls:**
- When a node with the given `id` already exists, the method returns the existing node and ignores the `path` parameter entirely. Calling `create("core.gain", "myOsc")` when a `core.oscillator` node named "myOsc" already exists returns the oscillator, not a gain node, with no warning.
- If the factory path does not match any registered factory, returns undefined with no error message. The only feedback is a message window popup shown on the message thread (not a script error).

**Cross References:**
- `$API.DspNetwork.createAndAdd$`
- `$API.DspNetwork.createFromJSON$`
- `$API.DspNetwork.get$`

**Example:**
```javascript:create-nodes
// Title: Creating nodes in a DspNetwork
const var nw = Engine.createDspNetwork("MyNetwork");

// Create a gain node with explicit ID
const var gain = nw.create("core.gain", "myGain");

// Existing ID returns the same node regardless of path
const var ref = nw.create("core.oscillator", "myGain"); // returns the gain, not an oscillator

// Auto-generate ID by passing empty string
const var osc = nw.create("core.oscillator", "");
```
```json:testMetadata:create-nodes
{
  "testable": false,
  "skipReason": "Requires a DspNetwork::Holder processor (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator)"
}
```

## createAndAdd

**Signature:** `ScriptObject createAndAdd(String path, String id, ScriptObject parent)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to create() (heap allocations) then calls setParent() which modifies the ValueTree hierarchy.
**Minimal Example:** `var node = {obj}.createAndAdd("core.gain", "myGain", parentNode);`

**Description:**
Convenience method that creates a node and immediately adds it as the last child of the given parent container. Equivalent to calling `create(path, id)` followed by `node.setParent(parent, -1)`. If `id` is empty, a unique name is auto-generated from the path suffix. Returns the created (or existing) node reference.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | String | no | Factory path in `factory.node` format | Must match a registered node factory |
| id | String | no | Unique node identifier | If empty, auto-generated from path suffix |
| parent | ScriptObject | no | Parent container node to add the new node to | Must be a container node (e.g. container.chain) |

**Cross References:**
- `$API.DspNetwork.create$`
- `$API.DspNetwork.createFromJSON$`
- `$API.Node.setParent$`

## createFromJSON

**Signature:** `ScriptObject createFromJSON(JSON jsonData, ScriptObject parent)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Recursively creates nodes via createAndAdd(), involving heap allocations, ValueTree construction, and string operations.
**Minimal Example:** `var node = {obj}.createFromJSON(nodeTree, parentNode);`

**Description:**
Recursively creates a tree of nodes from a JSON object and adds them to the given parent container. Each JSON object must have `FactoryPath` and `ID` properties. If the object has a `Nodes` array property, its elements are recursively processed as child nodes. Returns the top-level created node, or undefined if creation fails at any level.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | Node tree descriptor with FactoryPath, ID, and optional Nodes array | Must be a valid object with FactoryPath and ID properties |
| parent | ScriptObject | no | Parent container node to add the top-level node to | Must be a container node |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| FactoryPath | String | Factory path in `factory.node` format (e.g. `core.gain`) |
| ID | String | Unique node identifier |
| Nodes | Array | Optional array of child node descriptors (same format, recursive) |

**Cross References:**
- `$API.DspNetwork.create$`
- `$API.DspNetwork.createAndAdd$`

**Example:**
```javascript:create-from-json
// Title: Building a node tree from a JSON descriptor
const var nw = Engine.createDspNetwork("MyNetwork");
const var root = nw.get(nw.getId());

var tree = {
    "FactoryPath": "container.chain",
    "ID": "subChain",
    "Nodes": [
        { "FactoryPath": "core.gain", "ID": "gain1" },
        { "FactoryPath": "core.gain", "ID": "gain2" }
    ]
};

var result = nw.createFromJSON(tree, root);
```
```json:testMetadata:create-from-json
{
  "testable": false,
  "skipReason": "Requires a DspNetwork::Holder processor (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator)"
}
```

## createTest

**Signature:** `ScriptObject createTest(JSON testData)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptNetworkTest object. Backend-only (USE_BACKEND).
**Minimal Example:** `var test = {obj}.createTest({"SignalType": "Noise"});`

**Description:**
Creates a `ScriptNetworkTest` object for automated testing of this network. The `testData` JSON object is augmented with the network's ID (as `NodeId`) before creating the test. In exported plugins (frontend builds), this method is a no-op and returns undefined. The test object provides methods like `runTest()`, `setTestProperty()`, `setProcessSpecs()`, `expectEquals()`, `setWaitingTime()`, and `addRuntimeFunction()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| testData | JSON | no | Test configuration object | Must be a valid JSON object |

**Pitfalls:**
- Returns undefined silently in frontend (exported plugin) builds with no error message. Code that calls `createTest()` in a shipped plugin will get undefined and any subsequent method calls on the result will fail.

**Cross References:**
- `$API.ScriptNetworkTest$`

## deleteIfUnused

**Disabled:** no-op
**Disabled Reason:** Not registered via ADD_API_METHOD in the constructor. Exists as an internal C++ method called by clear() but is not callable from HISEScript. Present in the generated API reference due to its Doxygen comment in the header.

## get

**Signature:** `ScriptObject get(String id)`
**Return Type:** `ScriptObject`
**Call Scope:** warning
**Call Scope Note:** String involvement (atomic ref-count operations on toString() and ID comparisons). O(n) iteration over the network's node list.
**Minimal Example:** `var node = {obj}.get("myGain");`

**Description:**
Returns a reference to the node with the given ID. If the ID matches the network's own ID, returns the root node. If a Node object is passed instead of a string, it is returned as-is (pass-through). Returns undefined if no node with the given ID exists -- no error is thrown.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | Node identifier to look up. Also accepts a Node reference (returned as-is) | -- |

**Cross References:**
- `$API.DspNetwork.create$`

## prepareToPlay

**Signature:** `undefined prepareToPlay(Double sampleRate, Double blockSize)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires a write lock on the connection lock (if already initialized). Calls prepare() and reset() on the root node, which may allocate buffers and initialize DSP state.
**Minimal Example:** `{obj}.prepareToPlay(44100.0, 512);`

**Description:**
Initializes the network for audio processing at the given sample rate and block size. Runs pending post-init functions, stores the specs, prepares the root node, and marks the network as initialized. If `sampleRate` is zero or negative, the method returns without doing anything. The block size may be adjusted internally by `DynamicParameterModulationProperties` if a custom modulation block size is configured.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sampleRate | Double | no | Audio sample rate in Hz | Must be > 0.0 for initialization to proceed |
| blockSize | Double | no | Audio block size in samples | Positive value |

**Cross References:**
- `$API.DspNetwork.processBlock$`

## processBlock

**Signature:** `undefined processBlock(Array data)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** No allocations. Reads array/buffer pointers, constructs ProcessDataDyn on stack, delegates to process() which uses a non-blocking ScopedTryReadLock.
**Minimal Example:** `{obj}.processBlock([ch1, ch2]);`

**Description:**
Processes audio data through the network's node graph. The `data` parameter is an array of `Buffer` objects, one per channel. All buffers must have the same sample count. Internally converts the buffer array to a `ProcessDataDyn` struct and calls the network's `process()` method. If the network has not been initialized via `prepareToPlay`, processing is silently skipped. If the connection lock is held (network being modified), processing is also silently skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| data | Array | no | Array of Buffer objects, one per audio channel | All buffers must have equal sample counts |

**Pitfalls:**
- Silently produces no output if the network has not been initialized via `prepareToPlay()`. No error is thrown -- the buffers are simply left unmodified.

**Cross References:**
- `$API.DspNetwork.prepareToPlay$`

**Example:**
```javascript:process-block-manual
// Title: Manual DSP processing with channel buffers
const var nw = Engine.createDspNetwork("MyNetwork");

nw.prepareToPlay(44100.0, 512);

const var ch1 = Buffer.create(512);
const var ch2 = Buffer.create(512);

// Process stereo audio through the node graph
nw.processBlock([ch1, ch2]);
```
```json:testMetadata:process-block-manual
{
  "testable": false,
  "skipReason": "Requires a DspNetwork::Holder processor (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator)"
}
```

## setForwardControlsToParameters

**Signature:** `undefined setForwardControlsToParameters(Integer shouldForward)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a boolean flag. No allocations, no locks.
**Minimal Example:** `{obj}.setForwardControlsToParameters(true);`

**Description:**
Controls whether UI control values are forwarded directly to the network's root node parameters or routed through regular script callbacks. When enabled (default), the `NetworkParameterHandler` bridges root node parameters to the host's parameter system, making them available for DAW automation. When disabled, UI controls use the standard script callback path instead.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldForward | Integer | no | If true, UI controls drive network parameters directly | Boolean: 0 or 1 |

**Cross References:**
- `$API.DspNetwork.setParameterDataFromJSON$`

## setParameterDataFromJSON

**Signature:** `Integer setParameterDataFromJSON(JSON jsonData)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs string operations (toString, substring extraction), node/parameter lookups, and ValueTree property changes with undo manager.
**Minimal Example:** `{obj}.setParameterDataFromJSON({"myGain.Gain": -6.0});`

**Description:**
Sets multiple node parameters from a JSON object. Each property key uses `nodeId.parameterId` format (e.g. `"myGain.Gain"`). The value is cast to double. Matching nodes and parameters are updated via their ValueTree `Value` property with undo support. Successfully matched parameters are also marked as "probed" (for the probed parameter list feature). The return value is always true regardless of whether any parameters were matched (see pitfall).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | Object with `nodeId.parameterId` keys and numeric values | Keys must use dot-separated `nodeId.parameterId` format |

**Pitfalls:**
- [BUG] Always returns true regardless of whether any parameters were actually found and set. The implementation computes an `ok` flag that tracks whether at least one parameter was matched, but returns `true` unconditionally instead of returning `ok`. Unmatched node IDs or parameter IDs are silently ignored.

**Cross References:**
- `$API.Node.setParameterWithString$`
- `$API.DspNetwork.setForwardControlsToParameters$`
- `$API.DspNetwork.undo$`

**Example:**
```javascript:set-params-from-json
// Title: Setting multiple node parameters from a JSON object
const var nw = Engine.createDspNetwork("MyNetwork");

// Keys use "nodeId.parameterId" format
nw.setParameterDataFromJSON({
    "myGain.Gain": -6.0,
    "myOsc.Frequency": 440.0,
    "myFilter.Cutoff": 2000.0
});
```
```json:testMetadata:set-params-from-json
{
  "testable": false,
  "skipReason": "Requires a DspNetwork::Holder processor (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator)"
}
```

## undo

**Signature:** `Integer undo()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Calls UndoManager::undo() which replays ValueTree property changes, potentially triggering listeners and allocations.
**Minimal Example:** `{obj}.undo();`

**Description:**
Undoes the last action in the network's undo history. Uses the internal JUCE `UndoManager` which groups operations into transactions every 1500ms. Undo is enabled by default in the HISE backend (IDE) but disabled in exported plugins. Returns true if an action was successfully undone.

**Cross References:**
- `$API.DspNetwork.setParameterDataFromJSON$`

**Pitfalls:**
- [BUG] Crashes with a nullptr dereference when called in frontend (exported plugin) builds where undo is disabled. The method calls `getUndoManager(true)->undo()` without null-checking, and `getUndoManager()` returns nullptr when `enableUndo` is false.
