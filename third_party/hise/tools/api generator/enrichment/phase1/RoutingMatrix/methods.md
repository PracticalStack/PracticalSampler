# RoutingMatrix -- Methods

## addConnection

**Signature:** `bool addConnection(int sourceIndex, int destinationIndex)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock write lock on MatrixData; triggers sendChangeMessage for listener notification.
**Minimal Example:** `var ok = {obj}.addConnection(0, 2);`

**Description:**
Adds a primary channel connection, routing audio from `sourceIndex` to `destinationIndex`. Returns `true` if the connection was successfully added. Each source channel maps to at most one destination; calling `addConnection` on an already-connected source overwrites the previous destination.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceIndex | Number | no | Source channel index | 0 to numSourceChannels-1 |
| destinationIndex | Number | no | Destination channel index | 0 to numDestinationChannels-1 |

**Pitfalls:**
- When `numAllowedConnections` is 2 (default stereo), adding a third connection automatically removes an existing connection on the same even/odd side. After `setNumChannels(n)`, the constraint relaxes to `n` allowed connections.

**Cross References:**
- `$API.RoutingMatrix.removeConnection$`
- `$API.RoutingMatrix.addSendConnection$`

## addSendConnection

**Signature:** `bool addSendConnection(int sourceIndex, int destinationIndex)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock write lock on MatrixData; triggers sendChangeMessage.
**Minimal Example:** `var ok = {obj}.addSendConnection(0, 2);`

**Description:**
Adds a send connection, routing a copy of the audio from `sourceIndex` to `destinationIndex` in the parallel send bus. Send connections operate independently of primary connections, allowing the same source to feed both a main destination and a send destination simultaneously.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceIndex | Number | no | Source channel index | 0 to numSourceChannels-1 |
| destinationIndex | Number | no | Send destination channel index | 0 to numDestinationChannels-1 |

**Cross References:**
- `$API.RoutingMatrix.removeSendConnection$`
- `$API.RoutingMatrix.addConnection$`

## clear

**Signature:** `void clear()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock write lock multiple times (resetToDefault + two removeConnection calls); triggers sendChangeMessage.
**Minimal Example:** `{obj}.clear();`

**Description:**
Removes all primary and send connections. Internally calls `resetToDefault()` (which sets stereo passthrough 0->0, 1->1) then explicitly removes those default connections.

**Pitfalls:**
- When `numAllowedConnections` is 2 (default stereo constraint), the internal `removeConnection` calls trigger auto-correction that may re-add a default passthrough connection. The matrix may not end up truly empty unless `setNumChannels` was called first to relax the constraint.

**Cross References:**
- `$API.RoutingMatrix.removeConnection$`
- `$API.RoutingMatrix.setNumChannels$`
- `$API.RoutingMatrix.addConnection$`
- `$API.RoutingMatrix.addSendConnection$`

## getDestinationChannelForSource

**Signature:** `var getDestinationChannelForSource(var sourceIndex)`
**Return Type:** `Integer` or `Array`
**Call Scope:** warning
**Call Scope Note:** No lock acquired for single-value queries (reads fixed-size array). Array input mode constructs a new Array (heap allocation).
**Minimal Example:** `var dest = {obj}.getDestinationChannelForSource(0);`

**Description:**
Returns the destination channel index that the given source channel is connected to via the primary connection. Returns -1 if the source is not connected. Accepts either a single index or an array of indices; array input produces an array of results via per-element recursive calls.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceIndex | Number or Array | no | Source channel index or array of indices | 0 to numSourceChannels-1 per element |

**Cross References:**
- `$API.RoutingMatrix.getSourceChannelsForDestination$`
- `$API.RoutingMatrix.addConnection$`

## getNumDestinationChannels

**Signature:** `int getNumDestinationChannels()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var numDest = {obj}.getNumDestinationChannels();`

**Description:**
Returns the current number of destination (output) channels in the routing matrix. Unlike the `NumOutputs` constant (which is a snapshot from construction time), this reflects the live value.

**Cross References:**
- `$API.RoutingMatrix.getNumSourceChannels$`
- `$API.RoutingMatrix.setNumChannels$`

## getNumSourceChannels

**Signature:** `int getNumSourceChannels()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var numSrc = {obj}.getNumSourceChannels();`

**Description:**
Returns the current number of source (input) channels in the routing matrix. Unlike the `NumInputs` constant (which is a snapshot from construction time), this reflects the live value and updates after `setNumChannels` is called.

**Cross References:**
- `$API.RoutingMatrix.getNumDestinationChannels$`
- `$API.RoutingMatrix.setNumChannels$`

## getSourceChannelsForDestination

**Signature:** `var getSourceChannelsForDestination(var destinationIndex)`
**Return Type:** `Integer` or `Array`
**Call Scope:** unsafe
**Call Scope Note:** May construct a new Array when multiple sources map to the same destination (heap allocation even for single-value input).
**Minimal Example:** `var sources = {obj}.getSourceChannelsForDestination(0);`

**Description:**
Returns the source channel(s) connected to the given destination via primary connections. For a single destination index: returns -1 if no source is connected, a single integer if exactly one source maps to it, or an array of integers if multiple sources fan-in to the same destination. Accepts an array of destination indices to query multiple destinations at once, returning an array of results.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| destinationIndex | Number or Array | no | Destination channel index or array of indices | 0 to numDestinationChannels-1 per element |

**Pitfalls:**
- The return type varies depending on the connection state: -1 (Integer, no connection), a single Integer (one source), or an Array (multiple sources in fan-in). Always check the type before using the result as an array index.

**Cross References:**
- `$API.RoutingMatrix.getDestinationChannelForSource$`
- `$API.RoutingMatrix.addConnection$`

## getSourceGainValue

**Signature:** `float getSourceGainValue(int channelIndex)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Uses ScopedTryReadLock (non-blocking). Returns 0.0 if the lock cannot be acquired.
**Minimal Example:** `var peak = {obj}.getSourceGainValue(0);`

**Description:**
Returns the current peak level for the given source channel as a linear gain value. The value reflects the most recent audio peak magnitude, smoothed by internal decay coefficients (upDecayFactor=1.0, downDecayFactor=0.97).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channelIndex | Number | no | Source channel index | 0 to numSourceChannels-1 |

**Pitfalls:**
- Peak values are only computed when the routing editor is actively displayed in the HISE IDE. The internal `handleDisplayValues` method only runs when `anyChannelActive()` returns true, which requires at least one editor reference count. In exported plugins or when no editor is shown, all channels return 0.0. There is no script-level API to force peak metering.

**Cross References:**
- `$API.RoutingMatrix.getNumSourceChannels$`

## removeConnection

**Signature:** `bool removeConnection(int sourceIndex, int destinationIndex)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock write lock; triggers sendChangeMessage.
**Minimal Example:** `var ok = {obj}.removeConnection(0, 2);`

**Description:**
Removes the primary channel connection between `sourceIndex` and `destinationIndex`. Returns `true` if the connection existed and was removed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceIndex | Number | no | Source channel index | 0 to numSourceChannels-1 |
| destinationIndex | Number | no | Destination channel index | 0 to numDestinationChannels-1 |

**Pitfalls:**
- When `numAllowedConnections` is 2 (default stereo), removing a connection that drops the count below 2 auto-restores a default passthrough connection (`channelConnections[index] = index`). The matrix maintains at least a stereo pair under the default constraint.

**Cross References:**
- `$API.RoutingMatrix.addConnection$`
- `$API.RoutingMatrix.clear$`

## removeSendConnection

**Signature:** `bool removeSendConnection(int sourceIndex, int destinationIndex)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock write lock; triggers sendChangeMessage.
**Minimal Example:** `var ok = {obj}.removeSendConnection(0, 2);`

**Description:**
Removes a send connection between `sourceIndex` and `destinationIndex`. Returns `true` if the send connection existed and was removed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sourceIndex | Number | no | Source channel index | 0 to numSourceChannels-1 |
| destinationIndex | Number | no | Send destination channel index | 0 to numDestinationChannels-1 |

**Cross References:**
- `$API.RoutingMatrix.addSendConnection$`
- `$API.RoutingMatrix.clear$`

## setForcePeakMeters

**Disabled:** no-op
**Disabled Reason:** This method has no C++ implementation anywhere in the HISE source. It is not declared in the header, not implemented, and not registered in the constructor. It appears in the base JSON but was never implemented.

## setNumChannels

**Signature:** `void setNumChannels(int numSourceChannels)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock write lock; triggers processor numSourceChannelsChanged() callback and sendChangeMessage.
**Minimal Example:** `{obj}.setNumChannels(8);`

**Description:**
Sets the number of source channels and the maximum number of allowed connections for the routing matrix. Internally sets both `numSourceChannels` and `numAllowedConnections` to the provided value, which relaxes the default stereo constraint for multichannel routing configurations.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numSourceChannels | Number | no | Number of source channels to configure | 0-16 (validated against NUM_MAX_CHANNELS) |

**Pitfalls:**
- Throws "Can't resize this matrix" if the processor's matrix has `resizeAllowed == false` (the default for most processors). Only processors that explicitly enable resizing support this method.
- Sets source channels and `numAllowedConnections` but does NOT change `numDestinationChannels`. The destination count is determined by the processor's parent context.
- The `NumInputs` and `NumOutputs` constants are snapshot values from construction time and do NOT update. Use `getNumSourceChannels()` to read the live channel count after calling `setNumChannels`.

**Cross References:**
- `$API.RoutingMatrix.getNumSourceChannels$`
- `$API.RoutingMatrix.clear$`
