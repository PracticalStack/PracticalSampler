# Connection -- Method Analysis

## disconnect

**Signature:** `void disconnect()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Removes a ValueTree child with UndoManager, which involves heap operations and potential listener notifications.
**Minimal Example:** `{obj}.disconnect();`

**Description:**
Removes this connection from the scriptnode graph. The underlying ValueTree entry is removed from its parent tree (either a Parameter's Connections tree or a ModulationTargets tree), which triggers the DSP parameter chain rebuild on the source node. After calling disconnect, `isConnected()` returns false and `getSourceNode()` returns undefined.

**Parameters:**
None.

**Pitfalls:**
- After disconnecting, the Connection object still exists in script but is invalidated. Calling `getSourceNode()` returns undefined, though `getTarget()` may still return the Parameter object since it checks the WeakReference independently of connection state.

**Cross References:**
- `$API.Connection.isConnected$`
- `$API.Node.connectTo$`
- `$API.Parameter.addConnectionFrom$`

## getConnectionType

**Signature:** `int getConnectionType()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var type = {obj}.getConnectionType();`

**Description:**
Returns an integer indicating the connection source type. Intended to distinguish between macro parameter connections (0), single-output modulation connections (1), and multi-output modulation connections (2). However, the underlying `type` member is never assigned in the constructor or elsewhere, so the return value is indeterminate.

**Parameters:**
None.

**Pitfalls:**
- [BUG] The `type` member field is never assigned anywhere in the ConnectionBase class. The constructor does not set it, and no external code writes to it after construction. The returned value is uninitialized C++ data, making this method non-functional.

**Cross References:**
- `$API.Connection.getSourceNode$`
- `$API.Node.connectTo$`
- `$API.Parameter.addConnectionFrom$`

## getSourceNode

**Signature:** `var getSourceNode(int getSignalSource)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var src = {obj}.getSourceNode(true);`

**Description:**
Returns the source node of this connection. When `getSignalSource` is true, traces through cable intermediaries (e.g. `routing.local_cable`, `routing.global_cable`) to find the actual signal-producing node. When false, returns the immediate source node which may be a cable node acting as a routing intermediary. Returns undefined if the connection has been disconnected.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| getSignalSource | Integer | no | If true, traces through cable nodes to the real signal source. If false, returns the immediate source node. | Boolean (0 or 1) |

**Pitfalls:**
- Returns undefined (not null or an error) when the connection is disconnected. Always check `isConnected()` first if the connection may have been removed.

**Cross References:**
- `$API.Connection.getTarget$`
- `$API.Connection.isConnected$`

## getTarget

**Signature:** `var getTarget()`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var param = {obj}.getTarget();`

**Description:**
Returns the target Parameter object of this connection. The target is resolved during construction from the `NodeId` and `ParameterId` properties stored in the connection's ValueTree. Returns undefined if the target parameter was not found during construction or if the target node has been deleted.

**Parameters:**
None.

**Cross References:**
- `$API.Connection.getSourceNode$`
- `$API.Connection.isConnected$`
- `$API.Connection.disconnect$`
- `$API.Parameter.addConnectionFrom$`

## getUpdateRate

**Signature:** `int getUpdateRate()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var rate = {obj}.getUpdateRate();`

**Description:**
Returns the block size of the lowest common container ancestor of the source and target nodes. This reflects how often the modulation value is updated -- containers with smaller block sizes process more frequently and update modulation values at a higher rate. Returns 0 if no common container was found (e.g. when the signal source trace failed or the connection is disconnected).

**Parameters:**
None.

**Cross References:**
- `$API.Connection.getSourceNode$`
- `$API.Connection.getTarget$`
- `$API.Connection.isConnected$`

## isConnected

**Signature:** `bool isConnected()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.isConnected();`

**Description:**
Returns whether this connection is still active in the scriptnode graph. Checks if the underlying ValueTree entry still has a valid parent. Returns false after `disconnect()` has been called or if the connected node was deleted, removing the connection's ValueTree from the graph.

**Parameters:**
None.

**Cross References:**
- `$API.Connection.disconnect$`
