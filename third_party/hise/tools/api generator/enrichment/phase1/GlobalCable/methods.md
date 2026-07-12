## getValue

**Signature:** `Double getValue()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValue();`

**Description:**
Returns the current cable value converted from the internal normalised range (0..1) back through the local input range set by `setRange()`, `setRangeWithSkew()`, or `setRangeWithStep()`. If no range has been set, the default identity range (0..1) applies and this behaves identically to `getValueNormalised()`.

**Parameters:**

(None)

**Cross References:**
- `$API.GlobalCable.getValueNormalised$`
- `$API.GlobalCable.setValue$`
- `$API.GlobalCable.setRange$`

---

## getValueNormalised

**Signature:** `Double getValueNormalised()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValueNormalised();`

**Description:**
Returns the raw normalised cable value between 0.0 and 1.0, bypassing the local input range. This reads the cable's internal `lastValue` directly. Returns 0.0 if the cable reference is invalid.

**Parameters:**

(None)

**Cross References:**
- `$API.GlobalCable.getValue$`
- `$API.GlobalCable.setValueNormalised$`

---

## setValue

**Signature:** `undefined setValue(Double inputWithinRange)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValue(50.0);`

**Description:**
Converts the input value from the local input range to normalised 0..1 using `convertTo0to1()`, then sends the normalised value to all cable targets. If no range has been set (default 0..1 identity), this behaves identically to `setValueNormalised()`. The internal cable value is clamped to 0..1.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| inputWithinRange | Double | no | The value within the configured input range to send through the cable. | Must be within the range set by `setRange`/`setRangeWithSkew`/`setRangeWithStep`; values outside are clamped after normalisation. |

**Pitfalls:**
- The value is only meaningful if a range has been configured with `setRange()`, `setRangeWithSkew()`, or `setRangeWithStep()`. Without a range, this is equivalent to `setValueNormalised()`.

**Cross References:**
- `$API.GlobalCable.setValueNormalised$`
- `$API.GlobalCable.getValue$`
- `$API.GlobalCable.setRange$`

**DiagramRef:** cable-dispatch

**Example:**
```javascript:basic
// Send a frequency value through a cable with a custom range
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("FreqCable");

cable.setRange(20.0, 20000.0);
cable.setValue(440.0);

Console.print("Value set: " + cable.getValue());
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": [
      "Value set: 440.0"
    ]
  }
}
```


---

## setValueNormalised

**Signature:** `undefined setValueNormalised(Double normalisedInput)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValueNormalised(0.5);`

**Description:**
Sends a normalised value (0..1) directly to all cable targets, bypassing the local input range. The value is clamped to 0..1 internally by the cable infrastructure.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| normalisedInput | Double | no | The normalised value to send. | 0.0 to 1.0; values outside this range are clamped. |

**Cross References:**
- `$API.GlobalCable.setValue$`
- `$API.GlobalCable.getValueNormalised$`

---

## sendData

**Signature:** `undefined sendData(NotUndefined dataToSend)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendData({"key": "value"});`

**Description:**
Serialises any HISEScript value (JSON objects, strings, arrays, buffers) to a binary stream and sends it to all cable targets that have registered a data callback. This uses the data channel, which is independent of the value channel. A recursion guard prevents this reference's own registered data callbacks from firing when it sends data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataToSend | NotUndefined | no | The data to send through the cable. Can be JSON, String, Array, Buffer, or any serialisable var type. | Must not be undefined. |

**Pitfalls:**
- This method allocates a `MemoryOutputStream` on the heap. Do NOT call from the audio thread or a synchronous cable callback.
- The data is serialised and deserialised on each send, so large objects incur a performance cost.

**Cross References:**
- `$API.GlobalCable.registerDataCallback$`

**Example:**
```javascript
// Send a JSON data chunk to all data callback listeners
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("DataCable");

cable.sendData({"noteNumber": 60, "velocity": 100});
```

---

## setRange

**Signature:** `undefined setRange(Double min, Double max)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setRange(0.0, 100.0);`

**Description:**
Sets the local input range for this cable reference using a minimum and maximum value. No step size or skew factor is applied (linear mapping). This range is used by `setValue()` and `getValue()` to convert between user-facing values and the internal normalised 0..1 space. Does not affect `setValueNormalised()` or `getValueNormalised()`. Registered value callbacks also receive values converted through this range.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| min | Double | no | The minimum value of the input range. | |
| max | Double | no | The maximum value of the input range. | Must be greater than min. |

**Cross References:**
- `$API.GlobalCable.setRangeWithSkew$`
- `$API.GlobalCable.setRangeWithStep$`
- `$API.GlobalCable.setValue$`
- `$API.GlobalCable.getValue$`

---

## setRangeWithSkew

**Signature:** `undefined setRangeWithSkew(Double min, Double max, Double midPoint)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setRangeWithSkew(20.0, 20000.0, 1000.0);`

**Description:**
Sets the local input range with a skew factor derived from a mid point. The `midPoint` value specifies which input value should map to 0.5 in the normalised space, creating a logarithmic or exponential curve. Useful for frequency or gain ranges where perceptual linearity matters.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| min | Double | no | The minimum value of the input range. | |
| max | Double | no | The maximum value of the input range. | Must be greater than min. |
| midPoint | Double | no | The input value that maps to 0.5 in normalised space. | Must be between min and max. |

**Cross References:**
- `$API.GlobalCable.setRange$`
- `$API.GlobalCable.setRangeWithStep$`

**Example:**
```javascript:basic
// Set a frequency range with perceptual skew
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("FreqCable");

// 1000 Hz maps to the midpoint (0.5) of the normalised range
cable.setRangeWithSkew(20.0, 20000.0, 1000.0);
cable.setValue(1000.0);

Console.print("Normalized: " + cable.getValueNormalised());
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": [
      "Normalized: 0.5"
    ]
  }
}
```


---

## setRangeWithStep

**Signature:** `undefined setRangeWithStep(Double min, Double max, Double stepSize)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setRangeWithStep(0.0, 127.0, 1.0);`

**Description:**
Sets the local input range with a step size for quantised values. The output of `getValue()` will snap to multiples of the step size within the range.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| min | Double | no | The minimum value of the input range. | |
| max | Double | no | The maximum value of the input range. | Must be greater than min. |
| stepSize | Double | no | The step size for value quantisation. | Must be greater than 0. |

**Cross References:**
- `$API.GlobalCable.setRange$`
- `$API.GlobalCable.setRangeWithSkew$`

---

## registerCallback

**Signature:** `undefined registerCallback(Function callbackFunction, Integer synchronous)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.registerCallback(onCableValue, AsyncNotification);`

**Description:**
Registers a function to be called whenever a value is sent through the cable. The callback receives a single argument: the cable value converted through the local input range. Multiple callbacks can be registered per cable reference.

When `synchronous` is `SyncNotification`, the callback executes immediately on the calling thread (which may be the audio thread). The function must be realtime-safe (an `inline function`). If the function is not realtime-safe, the registration silently fails and the callback never fires.

When `synchronous` is `AsyncNotification`, the callback executes asynchronously on the UI thread via `PooledUIUpdater` timer polling. Rapid value changes are coalesced -- only the most recent value is delivered, intermediate values are dropped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackFunction | Function | no | The function to call when a value is received. Receives one argument: the converted value. | Must be a valid JavaScript function. For synchronous mode, must be an `inline function`. |
| synchronous | Integer | no | Whether the callback runs synchronously on the calling thread (`SyncNotification`) or asynchronously on the UI thread (`AsyncNotification`). | `SyncNotification` or `AsyncNotification`. |

**Callback Signature:** callbackFunction(value: double)

**Pitfalls:**
- Asynchronous callbacks coalesce values. If multiple `setValue()` calls happen between UI timer ticks, only the last value is delivered.
- Each call to `registerCallback` adds a new callback. There is no automatic replacement -- call `deregisterCallback` first to remove an existing one.

**Cross References:**
- `$API.GlobalCable.deregisterCallback$`
- `$API.GlobalCable.registerDataCallback$`

**Diagram:**
- **Brief:** Sync vs Async Callback Dispatch
- **Type:** timing
- **Description:** When synchronous=true, the callback function executes immediately inline on whatever thread called setValue/setValueNormalised (may be audio thread). When synchronous=false, the value is stored atomically via ModValue, and a PooledUIUpdater SimpleTimer polls on the UI thread at the display refresh rate, delivering only the latest value. Intermediate values between timer ticks are dropped.

**Example:**
```javascript:sync-and-async-callbacks
// Register both sync and async callbacks
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("MyCable");

cable.setRange(0.0, 1.0);

// Async callback (safe, runs on UI thread)
inline function onCableAsync(value)
{
    Console.print("Async: " + value);
};

cable.registerCallback(onCableAsync, AsyncNotification);

// Sync callback (runs on calling thread, must be realtime-safe)
inline function onCableSync(value)
{
    // Only do realtime-safe operations here
};

cable.registerCallback(onCableSync, SyncNotification);

// --- test-only ---
reg syncResult = -1.0;
inline function testSyncCapture(v) { syncResult = v; };
cable.registerCallback(testSyncCapture, SyncNotification);
cable.setValue(0.5);
// --- end test-only ---
```
```json:testMetadata:sync-and-async-callbacks
{
  "testable": true,
  "verifyScript": [
    {
      "expression": "syncResult",
      "value": 0.5
    }
  ]
}
```

---

## registerDataCallback

**Signature:** `undefined registerDataCallback(Function dataCallbackFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.registerDataCallback(onCableData);`

**Description:**
Registers a function to be called asynchronously when data is sent through the cable via `sendData()`. The callback receives one argument: the deserialised data (JSON, String, Array, Buffer, etc.). Data callbacks operate on the data channel, which is independent of the value channel. Multiple data callbacks can be registered per cable reference.

The callback is always asynchronous (high-priority via `WeakCallbackHolder`). A recursion guard prevents a reference's own data callback from firing when it sends data via `sendData()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataCallbackFunction | Function | no | The function to call when data is received. Receives one argument: the deserialised data. | Must be a valid JavaScript function. |

**Callback Signature:** dataCallbackFunction(data: var)

**Cross References:**
- `$API.GlobalCable.sendData$`
- `$API.GlobalCable.deregisterCallback$`
- `$API.GlobalCable.registerCallback$`

**Example:**
```javascript:data-callback-json
// Register a data callback to receive JSON chunks
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("DataCable");

inline function onDataReceived(data)
{
    Console.print("Received note: " + data.noteNumber);
};

cable.registerDataCallback(onDataReceived);

// --- test-only ---
// Use a second reference to bypass the recursion guard
const var triggerCable = rm.getCable("DataCable");
triggerCable.sendData({"noteNumber": 42});
// --- end test-only ---
```
```json:testMetadata:data-callback-json
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": ["Received note: 42"]
  }
}
```

---

## deregisterCallback

**Signature:** `Integer deregisterCallback(Function callbackFunction)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Minimal Example:** `var ok = {obj}.deregisterCallback(onCableValue);`

**Description:**
Removes a previously registered callback (either value or data callback) from this cable reference. Searches both the data callback list and the value callback list. Returns `true` if the callback was found and removed, `false` if not found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackFunction | Function | no | The function reference that was previously registered via `registerCallback` or `registerDataCallback`. | Must be the same function reference used during registration. |

**Cross References:**
- `$API.GlobalCable.registerCallback$`
- `$API.GlobalCable.registerDataCallback$`

---

## connectToMacroControl

**Signature:** `undefined connectToMacroControl(Integer macroIndex, Integer macroIsTarget, Integer filterRepetitions)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.connectToMacroControl(0, true, true);`

**Description:**
Connects the cable to a macro control so that cable values are forwarded to the macro. The cable's normalised 0..1 value is scaled to 0..127 for the macro system. Pass `macroIndex` as -1 to remove all existing macro connections from this cable.

Currently only `macroIsTarget=true` (cable drives macro) is implemented. The reverse direction (macro drives cable) is not yet supported.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| macroIndex | Integer | no | The zero-based macro index (0-7). Pass -1 to remove all macro connections. | -1 to 7. |
| macroIsTarget | Integer | no | Whether the macro receives values from the cable (`true`). Must be `true` -- the reverse is not implemented. | Must be `true`. |
| filterRepetitions | Integer | no | When `true`, consecutive identical values are filtered and not sent to the macro. | `true` or `false`. |

**Pitfalls:**
- Passing `macroIsTarget=false` triggers a debug assertion and does nothing. Always pass `true`.
- Macro values are scaled 0..1 -> 0..127 internally. The cable's input range does not affect this -- the raw normalised value is used.

**Cross References:**
- `$API.GlobalCable.connectToModuleParameter$`
- `$API.GlobalCable.connectToGlobalModulator$`

**Example:**
```javascript:basic
// Connect a cable to Macro 1, filtering repeated values
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("MacroCable");

cable.connectToMacroControl(0, true, true);
cable.setValueNormalised(0.5); // sets Macro 1 to 63.5 (0.5 * 127)
```
```json:testMetadata:basic
{
  "testable": false
}
```


---

## connectToGlobalModulator

**Signature:** `undefined connectToGlobalModulator(String lfoId, Integer addToMod)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.connectToGlobalModulator("GlobalLFO", true);`

**Description:**
Connects the cable to a global modulator (LFO, envelope, or voice start modulator) inside a `GlobalModulatorContainer` as a source. When connected, the modulator's output value is sent to the cable each processing block. The modulator must be a child of a `GlobalModulatorContainer`. Pass `addToMod=false` to disconnect.

Different modulator types are handled automatically: time-variant modulators send their last constant value, voice-start modulators send the voice start value, and envelope modulators send per-voice envelope data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lfoId | String | no | The processor ID of the modulator inside the GlobalModulatorContainer. | Must match a modulator name that is a child of a GlobalModulatorContainer. |
| addToMod | Integer | no | `true` to connect the modulator to the cable, `false` to disconnect. | `true` or `false`. |

**Pitfalls:**
- [BUG] The modulator must be inside a `GlobalModulatorContainer`. If the modulator exists but its parent is not a `GlobalModulatorContainer`, the call silently does nothing.

**Cross References:**
- `$API.GlobalCable.connectToMacroControl$`
- `$API.GlobalCable.connectToModuleParameter$`

---

## connectToModuleParameter

**Signature:** `undefined connectToModuleParameter(String processorId, NotUndefined parameterIndexOrId, NotUndefined targetObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.connectToModuleParameter("SimpleGain", "Gain", {"MinValue": -100, "MaxValue": 0});`

**Description:**
Connects the cable to a specific parameter of a HISE module. The cable's normalised 0..1 value is converted through the target range defined in `targetObject` before being applied to the module parameter. The target range JSON supports optional value smoothing via a `SmoothingTime` property.

The parameter can be specified by index (integer) or by name (string). Passing `parameterIndexOrId` as -1 with a valid `processorId` removes all connections for that processor. Passing an empty `processorId` with -1 removes all module parameter connections from the cable.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| processorId | String | no | The processor ID of the target module. Pass an empty string with parameterIndexOrId=-1 to clear all module connections. | Must match a processor name, or empty string for clearing. |
| parameterIndexOrId | NotUndefined | no | The parameter index (integer) or parameter name (string). Pass -1 to remove connections. | Valid parameter index or name for the target processor, or -1 for removal. |
| targetObject | NotUndefined | no | A JSON object defining the target range and optional smoothing time. | JSON object with range properties. |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| MinValue | Double | Minimum value of the target parameter range. |
| MaxValue | Double | Maximum value of the target parameter range. |
| StepSize | Double | Step size for value quantisation (optional). |
| SkewFactor | Double | Skew factor for non-linear mapping (optional). |
| SmoothingTime | Double | Smoothing time in milliseconds applied to value changes (optional, default 0). |

**Cross References:**
- `$API.GlobalCable.connectToMacroControl$`
- `$API.GlobalCable.connectToGlobalModulator$`

**Example:**
```javascript
// Connect a cable to a SimpleGain's Gain parameter with smoothing
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("GainCable");

cable.connectToModuleParameter("SimpleGain", "Gain", {
    "MinValue": -100.0,
    "MaxValue": 0.0,
    "SkewFactor": 5.0,
    "SmoothingTime": 50.0
});

cable.setValueNormalised(0.75);
```
