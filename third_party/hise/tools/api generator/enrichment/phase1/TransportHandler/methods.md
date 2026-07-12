# TransportHandler -- Methods

## setOnTempoChange

**Signature:** `undefined setOnTempoChange(Number sync, Function f)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new Callback object (heap allocation via `new Callback(...)`) and stores it in a ScopedPointer.
**Minimal Example:** `{obj}.setOnTempoChange(SyncNotification, onTempoChanged);`

**Description:**
Registers a callback that fires whenever the host tempo (BPM) changes. The `sync` parameter controls dispatch mode: `SyncNotification` executes the callback on the audio thread (requires `inline function`), `AsyncNotification` dispatches to the UI thread via PooledUIUpdater. The callback receives one argument: the new tempo as a double. Upon registration, the callback fires immediately with the current tempo value. Registering a sync callback clears any async callback using the same function reference (and vice versa).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sync | Number | yes | Dispatch mode for the callback. | `SyncNotification`, `AsyncNotification`, or `AsyncHiPriorityNotification` |
| f | Function | yes | Callback function to invoke on tempo change. | Must be `inline function` if sync is `SyncNotification`. Must accept 1 parameter. |

**Callback Signature:** f(newTempo: double)

**Pitfalls:**
- The callback fires immediately upon registration with the current BPM value (via forceSync=true), even if registered as async. This initial fire is always synchronous regardless of the dispatch mode.

**Cross References:**
- `$API.TransportHandler.setOnTransportChange$`
- `$API.TransportHandler.setOnSignatureChange$`

**DiagramRef:** sync-async-dispatch

**Example:**
```javascript:basic
// --- setup ---
Engine.setHostBpm(120);
// --- end setup ---
const var th = Engine.createTransportHandler();

var tempoLog = [];

inline function onTempoChanged(newTempo)
{
    tempoLog.push(newTempo);
}

th.setOnTempoChange(SyncNotification, onTempoChanged);

// --- test-only ---
Engine.setHostBpm(140);
// --- end test-only ---
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tempoLog.length", "value": 2},
    {"type": "REPL", "expression": "tempoLog[0]", "value": 120},
    {"type": "REPL", "expression": "tempoLog[1]", "value": 140}
  ]
}
```


## setOnTransportChange

**Signature:** `undefined setOnTransportChange(Number sync, Function f)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new Callback object (heap allocation via `new Callback(...)`).
**Minimal Example:** `{obj}.setOnTransportChange(AsyncNotification, onTransportChanged);`

**Description:**
Registers a callback that fires when the transport state changes (play/stop). The `sync` parameter controls dispatch mode. The callback receives one argument: a boolean indicating whether the transport is playing. Upon registration, the callback fires immediately with the current play state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sync | Number | yes | Dispatch mode for the callback. | `SyncNotification`, `AsyncNotification`, or `AsyncHiPriorityNotification` |
| f | Function | yes | Callback function to invoke on transport change. | Must be `inline function` if sync is `SyncNotification`. Must accept 1 parameter. |

**Callback Signature:** f(isPlaying: bool)

**Cross References:**
- `$API.TransportHandler.isPlaying$`
- `$API.TransportHandler.setOnTempoChange$`

**DiagramRef:** sync-async-dispatch

**Example:**
```javascript:basic
// --- setup ---
const var th0 = Engine.createTransportHandler();
th0.setSyncMode(th0.InternalOnly);
th0.stopInternalClock(0);
// --- end setup ---
const var th = Engine.createTransportHandler();

var transportLog = [];

inline function onTransportChanged(isPlaying)
{
    transportLog.push(isPlaying);
}

th.setOnTransportChange(SyncNotification, onTransportChanged);

// Trigger a transport change programmatically
th.startInternalClock(0);
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "transportLog.length", "value": 2},
    {"type": "REPL", "expression": "transportLog[0]", "value": false},
    {"type": "REPL", "expression": "transportLog[1]", "value": true}
  ]
}
```

## setOnBeatChange

**Signature:** `undefined setOnBeatChange(Number sync, Function f)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new Callback object and registers/removes a MusicalUpdateListener.
**Minimal Example:** `{obj}.setOnBeatChange(SyncNotification, onBeatChanged);`

**Description:**
Registers a callback that fires on each musical beat. The callback receives two arguments: the beat index (integer) and a boolean indicating whether this is the first beat of a new bar. The beat callback takes the time signature denominator into account -- in 6/8 time it fires twice as often as in 3/4. Unlike tempo and transport callbacks, this does NOT fire immediately upon registration. Calling `setOnBeatChange` also registers the TransportHandler as a MusicalUpdateListener; passing `undefined` as the function removes the listener.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sync | Number | yes | Dispatch mode for the callback. | `SyncNotification`, `AsyncNotification`, or `AsyncHiPriorityNotification` |
| f | Function | yes | Callback function to invoke on beat change, or `undefined` to remove the listener. | Must be `inline function` if sync is `SyncNotification`. Must accept 2 parameters. |

**Callback Signature:** f(beatIndex: int, isNewBar: bool)

**Pitfalls:**
- Does not fire immediately upon registration (unlike `setOnTempoChange` and `setOnTransportChange`). The first callback comes at the next beat boundary.

**Cross References:**
- `$API.TransportHandler.setOnGridChange$`
- `$API.TransportHandler.setOnSignatureChange$`

**Example:**
```javascript:basic
const var th = Engine.createTransportHandler();

inline function onBeatChanged(beatIndex, isNewBar)
{
    if (isNewBar)
        Console.print("New bar at beat " + beatIndex);
}

th.setOnBeatChange(SyncNotification, onBeatChanged);
```
```json:testMetadata:basic
{
  "testable": false
}
```


## setOnGridChange

**Signature:** `undefined setOnGridChange(Number sync, Function f)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new Callback object and registers/removes a MusicalUpdateListener.
**Minimal Example:** `{obj}.setOnGridChange(SyncNotification, onGridChanged);`

**Description:**
Registers a callback that fires on each grid tick. The grid must be enabled first via `setEnableGrid()`. The callback receives three arguments: the grid index (integer, adjusted for the local multiplier), a timestamp (sample offset within the current audio block for sample-accurate scheduling), and a boolean indicating whether this is the first grid tick in the current playback session (or after a discontinuity). Like `setOnBeatChange`, this does NOT fire immediately upon registration. Passing `undefined` as the function removes the MusicalUpdateListener.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sync | Number | yes | Dispatch mode for the callback. | `SyncNotification`, `AsyncNotification`, or `AsyncHiPriorityNotification` |
| f | Function | yes | Callback function to invoke on grid change, or `undefined` to remove the listener. | Must be `inline function` if sync is `SyncNotification`. Must accept 3 parameters. |

**Callback Signature:** f(gridIndex: int, timestamp: int, firstGridInPlayback: bool)

**Pitfalls:**
- The grid must be enabled via `setEnableGrid(true, tempoFactor)` before this callback can fire. Without it, the callback is registered but never triggered -- no error is reported.
- Does not fire immediately upon registration.

**Cross References:**
- `$API.TransportHandler.setEnableGrid$`
- `$API.TransportHandler.setLocalGridMultiplier$`
- `$API.TransportHandler.setLocalGridBypassed$`
- `$API.TransportHandler.getGridPosition$`
- `$API.TransportHandler.getGridLengthInSamples$`

**DiagramRef:** sync-async-dispatch

**Example:**
```javascript:basic
const var th = Engine.createTransportHandler();

// Enable 1/16 note grid (index 11)
th.setEnableGrid(true, 11);

inline function onGridChanged(gridIndex, timestamp, firstGrid)
{
    if (firstGrid)
        Console.print("Grid restarted at index " + gridIndex);
}

th.setOnGridChange(SyncNotification, onGridChanged);
```
```json:testMetadata:basic
{
  "testable": false
}
```


## setOnSignatureChange

**Signature:** `undefined setOnSignatureChange(Number sync, Function f)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new Callback object (heap allocation via `new Callback(...)`).
**Minimal Example:** `{obj}.setOnSignatureChange(AsyncNotification, onSignatureChanged);`

**Description:**
Registers a callback that fires when the time signature changes. The callback receives two arguments: the numerator and denominator of the new time signature. Upon registration, the callback fires immediately with the current time signature.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sync | Number | yes | Dispatch mode for the callback. | `SyncNotification`, `AsyncNotification`, or `AsyncHiPriorityNotification` |
| f | Function | yes | Callback function to invoke on signature change. | Must be `inline function` if sync is `SyncNotification`. Must accept 2 parameters. |

**Callback Signature:** f(nominator: int, denominator: int)

**Cross References:**
- `$API.TransportHandler.setOnBeatChange$`

**Example:**
```javascript:basic
const var th = Engine.createTransportHandler();

inline function onSignatureChanged(nom, denom)
{
    Console.print("Time signature: " + nom + "/" + denom);
}

th.setOnSignatureChange(AsyncNotification, onSignatureChanged);
```
```json:testMetadata:basic
{
  "testable": false
}
```


## setOnBypass

**Signature:** `undefined setOnBypass(Function f)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new Callback object and registers with the plugin bypass handler.
**Minimal Example:** `{obj}.setOnBypass(onBypassChanged);`

**Description:**
Registers a callback that fires when the plugin's bypass state changes. This callback is always asynchronous (there is no sync parameter). The callback receives one argument: a boolean indicating whether the plugin is bypassed. The bypass handler fires the callback immediately upon registration with the current bypass state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| f | Function | yes | Callback function to invoke on bypass change. | Must accept 1 parameter. |

**Callback Signature:** f(isBypassed: bool)

**Example:**
```javascript:basic
const var th = Engine.createTransportHandler();

inline function onBypassChanged(isBypassed)
{
    Console.print(isBypassed ? "Plugin bypassed" : "Plugin active");
}

th.setOnBypass(onBypassChanged);
```
```json:testMetadata:basic
{
  "testable": false
}
```

## setSyncMode

**Signature:** `undefined setSyncMode(Integer syncMode)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setSyncMode({obj}.PreferExternal);`

**Description:**
Sets the sync mode for the global master clock. This controls how the internal clock (started/stopped by `startInternalClock`/`stopInternalClock`) interacts with the external DAW clock. Use the TransportHandler constants (`Inactive`, `ExternalOnly`, `InternalOnly`, `PreferInternal`, `PreferExternal`, `SyncInternal`) as the argument. This is a global setting -- it affects all TransportHandler instances.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| syncMode | Integer | no | The sync mode constant. | 0-5, use TransportHandler constants (e.g. `th.PreferExternal`) |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| Inactive (0) | All clock processing disabled. No transport, beat, or grid callbacks fire. `getPPQPos()` returns 0. |
| ExternalOnly (1) | Only DAW transport drives callbacks. Internal clock calls are accepted but ignored (no internal playhead created). Grid timing follows DAW PPQ position. |
| InternalOnly (2) | Only script-driven clock drives callbacks. DAW transport is completely ignored (`allowExternalSync()` returns false). Grid and PPQ use internal uptime counter. |
| PreferInternal (3) | Internal clock takes priority. When internal is playing, external events are dropped. When internal is idle, external events drive transport normally. Internal playhead always available. |
| PreferExternal (4) | DAW clock takes priority. When DAW is playing, internal start/stop events are dropped. When DAW starts, handoff from internal to external fires a grid resync. When DAW stops, internal clock resumes (unless `stopInternalClockOnExternalStop` is set). |
| SyncInternal (5) | Internal clock drives start/stop, but PPQ position continuously syncs to DAW position while DAW is playing. External stop commands are ignored -- internal clock keeps running. Combines internal control with external musical alignment. |

**Cross References:**
- `$API.TransportHandler.startInternalClock$`
- `$API.TransportHandler.stopInternalClock$`
- `$API.TransportHandler.setLinkBpmToSyncMode$`

**DiagramRef:** clock-sync-modes

## startInternalClock

**Signature:** `undefined startInternalClock(Integer timestamp)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.startInternalClock(0);`

**Description:**
Starts the internal master clock at the given sample timestamp offset within the current audio block. If called from within audio rendering, it immediately processes the grid and triggers transport callbacks for the current block. This is a global operation -- it affects the shared MasterClock.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timestamp | Integer | no | Sample offset within the current audio block for sub-block start accuracy. | 0 to current block size |

**Cross References:**
- `$API.TransportHandler.stopInternalClock$`
- `$API.TransportHandler.setSyncMode$`
- `$API.TransportHandler.setOnTransportChange$`

## stopInternalClock

**Signature:** `undefined stopInternalClock(Integer timestamp)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.stopInternalClock(0);`

**Description:**
Stops the internal master clock at the given sample timestamp offset within the current audio block. If called from within audio rendering, it immediately processes the grid and triggers transport callbacks for the current block. This is a global operation -- it affects the shared MasterClock.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timestamp | Integer | no | Sample offset within the current audio block for sub-block stop accuracy. | 0 to current block size |

**Cross References:**
- `$API.TransportHandler.startInternalClock$`
- `$API.TransportHandler.setSyncMode$`
- `$API.TransportHandler.setOnTransportChange$`

## stopInternalClockOnExternalStop

**Signature:** `undefined stopInternalClockOnExternalStop(Integer shouldStop)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.stopInternalClockOnExternalStop(true);`

**Description:**
Configures whether the internal clock should automatically stop when the external (DAW) clock stops. This is a global setting on the MasterClock. Useful when using `SyncInternal` mode to prevent the internal clock from continuing after the DAW stops.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldStop | Integer | no | Whether to stop the internal clock on external stop. | Boolean: true/false |

**Cross References:**
- `$API.TransportHandler.startInternalClock$`
- `$API.TransportHandler.stopInternalClock$`
- `$API.TransportHandler.setSyncMode$`

## setLinkBpmToSyncMode

**Signature:** `undefined setLinkBpmToSyncMode(Integer shouldPrefer)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setLinkBpmToSyncMode(true);`

**Description:**
When enabled, the BPM source (internal vs external) is automatically linked to the current sync mode. This means the BPM reported by the transport will match the active clock source determined by `setSyncMode()`. This is a global setting on the MasterClock.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldPrefer | Integer | no | Whether to link BPM source to the sync mode. | Boolean: true/false |

**Cross References:**
- `$API.TransportHandler.setSyncMode$`

## setEnableGrid

**Signature:** `undefined setEnableGrid(Integer shouldBeEnabled, Integer tempoFactor)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setEnableGrid(true, 7);`

**Description:**
Enables or disables the high-precision grid timer at a specific musical tempo division. The `tempoFactor` is an index into the TempoSyncer note value table (0-18). This is a global setting -- enabling the grid affects all TransportHandler instances. The grid must be enabled before `setOnGridChange` callbacks will fire.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeEnabled | Integer | no | Whether to enable the grid. | Boolean: true/false |
| tempoFactor | Integer | no | Index into the tempo note value table. | 0-18 (see table below). Reports script error if out of range. |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| 0 | 1/1 (Whole note) |
| 1 | 1/2D (Half note dotted) |
| 2 | 1/2 (Half note) |
| 3 | 1/2T (Half note triplet) |
| 4 | 1/4D (Quarter note dotted) |
| 5 | 1/4 (Quarter note) |
| 6 | 1/4T (Quarter note triplet) |
| 7 | 1/8D (Eighth note dotted) |
| 8 | 1/8 (Eighth note) |
| 9 | 1/8T (Eighth note triplet) |
| 10 | 1/16D (Sixteenth note dotted) |
| 11 | 1/16 (Sixteenth note) |
| 12 | 1/16T (Sixteenth note triplet) |
| 13 | 1/32D (32nd note dotted) |
| 14 | 1/32 (32nd note) |
| 15 | 1/32T (32nd note triplet) |
| 16 | 1/64D (64th note dotted) |
| 17 | 1/64 (64th note) |
| 18 | 1/64T (64th note triplet) |

**Pitfalls:**
- [BUG] The error message says "Use 1-18" but valid indices actually start at 0 (Whole note). Index 0 is valid.

**Cross References:**
- `$API.TransportHandler.setOnGridChange$`
- `$API.TransportHandler.setLocalGridMultiplier$`
- `$API.TransportHandler.getGridLengthInSamples$`

**Example:**
```javascript:basic
// --- setup ---
Engine.setHostBpm(120);
// --- end setup ---
const var th = Engine.createTransportHandler();

// Enable a 1/16 note grid
th.setEnableGrid(true, 11);

// Enable a 1/8 note grid (overwrites the previous)
th.setEnableGrid(true, 8);
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "th.getGridLengthInSamples()", "value": 11025}
}
```

## setLocalGridMultiplier

**Signature:** `undefined setLocalGridMultiplier(Integer factor)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setLocalGridMultiplier(4);`

**Description:**
Sets a per-instance multiplier that slows down the grid callbacks for this TransportHandler. Only every Nth grid tick is passed through, where N is the factor. The grid index in the callback is adjusted (divided by the factor). This is local to this TransportHandler instance -- other instances are not affected.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| factor | Integer | no | Grid rate divisor. | Must be 1 or a power of two (2, 4, 8, 16, 32, 64). Reports script error if not. Clamped to [1, 64]. |

**Cross References:**
- `$API.TransportHandler.setEnableGrid$`
- `$API.TransportHandler.setOnGridChange$`
- `$API.TransportHandler.getGridLengthInSamples$`

## setLocalGridBypassed

**Signature:** `undefined setLocalGridBypassed(Integer shouldBeBypassed)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setLocalGridBypassed(true);`

**Description:**
Bypasses the grid callback for this TransportHandler instance. When bypassed, no grid callbacks fire for this handler. When unbypassed, the next grid callback will have `firstGridInPlayback` set to `true`. This is local to this instance -- other TransportHandler instances are not affected.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Integer | no | Whether to bypass grid callbacks. | Boolean: true/false |

**Cross References:**
- `$API.TransportHandler.setOnGridChange$`
- `$API.TransportHandler.setEnableGrid$`

## sendGridSyncOnNextCallback

**Signature:** `undefined sendGridSyncOnNextCallback()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.sendGridSyncOnNextCallback();`

**Description:**
Forces the next grid callback to have its `firstGridInPlayback` argument set to `true`. This provides a manual resync point for the grid, useful when you need to reset sequencer state to the beginning. This is a global operation on the MasterClock -- it affects all TransportHandler instances.

**Cross References:**
- `$API.TransportHandler.setOnGridChange$`

## isPlaying

**Signature:** `Integer isPlaying()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var playing = {obj}.isPlaying();`

**Description:**
Returns whether the transport is currently playing (internal or external clock, depending on sync mode). Returns 1 if playing, 0 if stopped.

**Cross References:**
- `$API.TransportHandler.setOnTransportChange$`
- `$API.TransportHandler.startInternalClock$`

## isNonRealtime

**Signature:** `Integer isNonRealtime()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bouncing = {obj}.isNonRealtime();`

**Description:**
Returns whether the DAW is currently bouncing/exporting the audio to a file (non-realtime rendering). Returns 1 if bouncing, 0 if in realtime playback. Useful in transport change callbacks to adjust processing for offline rendering (e.g., disabling UI updates, using higher quality algorithms).

## getGridLengthInSamples

**Signature:** `Double getGridLengthInSamples()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var gridLen = {obj}.getGridLengthInSamples();`

**Description:**
Returns the duration of one grid tick in samples, based on the current BPM, the global grid tempo setting, the local grid multiplier, and the sample rate. This accounts for the local multiplier -- if the multiplier is 4, the returned length is 4x the base grid length. Useful for calculating buffer sizes or scheduling positions relative to the grid.

**Cross References:**
- `$API.TransportHandler.setEnableGrid$`
- `$API.TransportHandler.setLocalGridMultiplier$`

## getGridPosition

**Signature:** `Integer getGridPosition(Integer timestamp)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var pos = {obj}.getGridPosition(0);`

**Description:**
Returns the current PPQ position as an integer for the given sample timestamp offset. The timestamp is a sample offset from the start of the current audio block (0 = start of block). The returned value is the PPQ position truncated to an integer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timestamp | Integer | no | Sample offset within the current audio block. | 0 to current block size |

**Cross References:**
- `$API.TransportHandler.setOnGridChange$`
