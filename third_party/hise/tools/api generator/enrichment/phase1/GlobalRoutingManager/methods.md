## getCable

**Signature:** `ScriptObject getCable(String cableId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new GlobalCableReference wrapper object on each call.
**Minimal Example:** `var cable = {obj}.getCable("/volume");`

**Description:**
Returns a GlobalCable reference for the given cable ID. If no cable with that ID exists, it is created on demand. Multiple calls with the same ID return separate wrapper objects that reference the same underlying cable. Cable IDs starting with `/` are OSC-addressable; other IDs work only for internal routing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| cableId | String | no | Name identifier for the cable | Use `/` prefix for OSC-addressable cables |

**Cross References:**
- `$API.GlobalCable$`
- `$API.GlobalRoutingManager.connectToOSC$`

## connectToOSC

**Signature:** `Integer connectToOSC(JSON connectionData, Function errorFunction)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Creates OSC sender/receiver objects, performs network bind/connect operations, allocates callback holders.
**Minimal Example:** `{obj}.connectToOSC({"SourcePort": 9000, "TargetPort": 9001}, onOscError);`

**Description:**
Establishes a bidirectional OSC connection for external controller communication. The connectionData JSON configures the receiver bind address/port, optional sender target, domain prefix, and per-cable value ranges. If an errorFunction is provided, it receives OSC parsing errors as a single string argument. Calling again with different settings tears down the previous connection first. Any OSC callbacks registered via `addOSCCallback` before this call are automatically configured with the new domain. Setting `TargetPort` to -1 (or omitting it) creates a receive-only connection with no sender. Passing a non-function value for errorFunction clears any previously registered error handler.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| connectionData | JSON | no | OSC connection configuration object | See Callback Properties |
| errorFunction | Function | no | Error handler for OSC parsing errors | 1 argument; pass `false` to clear |

**Callback Signature:** errorFunction(errorMessage: String)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Domain | String | Root OSC address prefix (default: "/hise_osc_receiver"). Auto-prefixed with `/` if missing, trailing `/` stripped. |
| SourceURL | String | Receiver bind address (default: "127.0.0.1") |
| SourcePort | Integer | Receiver listen port (default: 9000) |
| TargetURL | String | Sender target address (default: "127.0.0.1") |
| TargetPort | Integer | Sender target port. -1 = receive-only, no sender created (default: -1) |
| Parameters | JSON | Map of cable IDs to range objects (`{"MinValue": n, "MaxValue": n}`) for OSC value normalisation |

**Pitfalls:**
- [BUG] Always returns false regardless of whether the OSC connection was successfully established. The internal success flag is used to decide whether to register the listener but is not propagated as the return value.

**Cross References:**
- `$API.GlobalRoutingManager.sendOSCMessage$`
- `$API.GlobalRoutingManager.addOSCCallback$`
- `$API.GlobalRoutingManager.removeOSCCallback$`

**Example:**
```javascript:osc-connection-setup
// Title: Setting up an OSC connection with parameter ranges
const var rm = Engine.getGlobalRoutingManager();

inline function onOscError(errorMessage)
{
    Console.print("OSC error: " + errorMessage);
};

rm.connectToOSC({
    "Domain": "/myPlugin",
    "SourcePort": 9000,
    "TargetPort": 9001,
    "TargetURL": "127.0.0.1",
    "Parameters": {
        "/volume": {"MinValue": -100.0, "MaxValue": 0.0},
        "/pan": {"MinValue": -1.0, "MaxValue": 1.0}
    }
}, onOscError);

// Cables with matching IDs now send/receive via OSC
const var cable = rm.getCable("/volume");
```
```json:testMetadata:osc-connection-setup
{
  "testable": false,
  "skipReason": "Requires network I/O and OSC port binding"
}
```

## addOSCCallback

**Signature:** `undefined addOSCCallback(String oscSubAddress, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an OSCCallback object and modifies the callback list.
**Minimal Example:** `{obj}.addOSCCallback("/fader", onOscMessage);`

**Description:**
Registers a callback function for incoming OSC messages matching the given sub-address. The sub-address is combined with the connection domain to form the full OSC address pattern. If `connectToOSC` has already been called, the full address is built immediately; otherwise it is built automatically when `connectToOSC` is called later. The callback executes on the OSC receiver thread with high priority -- it is not deferred to the UI thread.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| oscSubAddress | String | no | OSC sub-address to listen for | Combined with domain to form full address |
| callback | Function | no | Handler for matching OSC messages | 2 arguments |

**Callback Signature:** callback(subAddress: String, value: var)

**Pitfalls:**
- OSC callbacks execute on the OSC receiver thread, not the scripting UI thread. The callback should avoid heavy processing or UI state changes that are not thread-safe.
- For multi-argument OSC messages, the value parameter is an Array rather than a single value.

**Cross References:**
- `$API.GlobalRoutingManager.removeOSCCallback$`
- `$API.GlobalRoutingManager.connectToOSC$`

**Example:**
```javascript:osc-callback-registration
// Title: Registering OSC callbacks for incoming messages
const var rm = Engine.getGlobalRoutingManager();

inline function onFaderMessage(subAddress, value)
{
    Console.print("Fader " + subAddress + " = " + value);
};

// Register before or after connectToOSC -- order does not matter
rm.addOSCCallback("/fader1", onFaderMessage);
rm.addOSCCallback("/fader2", onFaderMessage);

rm.connectToOSC({
    "Domain": "/mixer",
    "SourcePort": 9000
}, false);
```
```json:testMetadata:osc-callback-registration
{
  "testable": false,
  "skipReason": "Requires external OSC messages to trigger the callback"
}
```

## removeOSCCallback

**Signature:** `Integer removeOSCCallback(String oscSubAddress)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Modifies the callback list (array removal).
**Minimal Example:** `{obj}.removeOSCCallback("/fader1");`

**Description:**
Removes the first OSC callback registered for the given sub-address. Returns true if a callback was found and removed, false if no callback matches the sub-address.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| oscSubAddress | String | no | The sub-address of the callback to remove | Must match the string used in addOSCCallback |

**Pitfalls:**
- Only removes the first matching callback. If multiple callbacks were registered for the same sub-address, call repeatedly until it returns false to remove all of them.

**Cross References:**
- `$API.GlobalRoutingManager.addOSCCallback$`
- `$API.GlobalRoutingManager.connectToOSC$`

## sendOSCMessage

**Signature:** `Integer sendOSCMessage(String oscSubAddress, NotUndefined data)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Constructs OSC message objects and performs network I/O via the JUCE OSCSender.
**Minimal Example:** `{obj}.sendOSCMessage("/volume", 0.75);`

**Description:**
Sends an OSC message to the configured target. The sub-address is appended to the connection domain to form the full OSC address. Returns true on success, false if no sender is connected (requires `connectToOSC` with a valid `TargetPort`). Data type conversion: doubles become OSC float32, integers and booleans become OSC int32, strings become OSC strings. An array sends a multi-argument OSC message with one argument per element.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| oscSubAddress | String | no | OSC sub-address appended to the domain | Must form a valid OSC address with the domain |
| data | NotUndefined | no | Value(s) to send | Double, Integer, String, or Array of these. Other types throw "illegal var type for OSC data". |

**Pitfalls:**
- Returns false with no error if no OSC sender is configured. Ensure `connectToOSC` was called with a valid `TargetPort` before sending.

**Cross References:**
- `$API.GlobalRoutingManager.connectToOSC$`
- `$API.GlobalRoutingManager.addOSCCallback$`

## setEventData

**Signature:** `undefined setEventData(Integer eventId, Integer dataSlot, Double value)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Direct write to a fixed-size array with lock-free broadcaster notification. No allocations or locks.
**Minimal Example:** `{obj}.setEventData(Message.getEventId(), 0, velocity);`

**Description:**
Stores a double value in the per-event data storage, keyed by MIDI event ID and slot index. The stored value can be retrieved with `getEventData` and is also accessible downstream by EventDataModulator, scriptnode routing nodes, and the ComplexGroupManager. The storage uses bitmask hashing with 1024 event slots and 16 data slots per event, making it suitable for realtime use in MIDI callbacks.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | The MIDI event ID | Obtained via Message.getEventId() in MIDI callbacks |
| dataSlot | Integer | no | The data slot index | 0-15 (masked to 4 bits internally) |
| value | Double | no | The value to store | Any double |

**Pitfalls:**
- Hash collisions can occur for event IDs that share the same lower 10 bits (eventId & 1023). A collision silently overwrites the previous entry. In practice this is rare since MIDI event IDs cycle sequentially.

**Cross References:**
- `$API.GlobalRoutingManager.getEventData$`

**Example:**
```javascript:event-data-storage
// Title: Attaching custom data to MIDI events
const var rm = Engine.getGlobalRoutingManager();

// Store values in two different data slots for event ID 1
rm.setEventData(1, 0, 0.75);
rm.setEventData(1, 1, 440.0);

// Read back stored values
var slot0 = rm.getEventData(1, 0);
var slot1 = rm.getEventData(1, 1);
var empty = rm.getEventData(1, 2);

Console.print("Slot 0: " + slot0); // Slot 0: 0.75
Console.print("Slot 1: " + slot1); // Slot 1: 440
Console.print("Slot 2: " + empty); // Slot 2: undefined
```
```json:testMetadata:event-data-storage
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "rm.getEventData(1, 0)", "value": 0.75},
    {"type": "REPL", "expression": "rm.getEventData(1, 1)", "value": 440.0},
    {"type": "REPL", "expression": "rm.getEventData(1, 2)", "value": "undefined"}
  ]
}
```

## getEventData

**Signature:** `Double getEventData(Integer eventId, Integer dataSlot)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Direct read from a fixed-size array. No allocations or locks.
**Minimal Example:** `var vel = {obj}.getEventData(Message.getEventId(), 0);`

**Description:**
Reads a value from the per-event data storage. Returns the stored double value if the event ID and slot match a written entry. Returns undefined if the slot was never written or if a hash collision occurred (a different event ID occupies the same hash bucket). The return type is Double on success, undefined on miss.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | The MIDI event ID | Obtained via Message.getEventId() in MIDI callbacks |
| dataSlot | Integer | no | The data slot index | 0-15 (masked to 4 bits internally) |

**Cross References:**
- `$API.GlobalRoutingManager.setEventData$`
