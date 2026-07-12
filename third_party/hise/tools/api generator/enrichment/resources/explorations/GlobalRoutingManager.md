# GlobalRoutingManager -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- survey entry for GlobalRoutingManager
- `enrichment/phase1/GlobalCable/Readme.md` -- prerequisite class (GlobalCable distillation)
- `enrichment/resources/explorations/GlobalRoutingManager.md` -- this file (self)

## Prerequisite Context: GlobalCable

GlobalCable (distilled in phase1/GlobalCable/Readme.md) describes the individual named data bus.
GlobalRoutingManager is the factory/singleton that owns all cables, manages OSC connections,
and provides the event data storage. The GlobalCable Readme covers cable internals (value/data
channels, target system, callback dispatch modes, input ranges). This exploration focuses on
the manager-level infrastructure: OSC subsystem, event data storage, and cable factory.

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:2681-2763`

```cpp
struct GlobalRoutingManagerReference : public ConstScriptingObject,
                                       public ControlledObject,
                                       public WeakErrorHandler,
                                       public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
```

### Inheritance

| Base | Purpose |
|------|---------|
| `ConstScriptingObject` | Scripting API object with constant number of properties (0 in this case) |
| `ControlledObject` | Access to `MainController` via `getMainController()` |
| `WeakErrorHandler` | Receives error messages (used for OSC error forwarding) |
| `OSCReceiver::Listener<RealtimeCallback>` | Receives OSC messages from the JUCE OSCReceiver in realtime mode |

### Object Name

Returns `"GlobalRoutingManager"` via `RETURN_STATIC_IDENTIFIER`.

---

## Constructor

**File:** `ScriptingApiObjects.cpp:8920-8935`

```cpp
GlobalRoutingManagerReference(ProcessorWithScriptingContent* sp):
    ConstScriptingObject(sp, 0),          // 0 constants
    ControlledObject(sp->getMainController_()),
    errorCallback(sp, this, var(), 1)
{
    auto ptr = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(getMainController());
    manager = ptr.get();

    ADD_API_METHOD_1(getCable);
    ADD_API_METHOD_2(connectToOSC);
    ADD_API_METHOD_2(sendOSCMessage);
    ADD_API_METHOD_2(addOSCCallback);
    ADD_API_METHOD_1(removeOSCCallback);
    ADD_API_METHOD_3(setEventData);
    ADD_API_METHOD_2(getEventData);
}
```

**Key observations:**
- 0 constants added (`ConstScriptingObject(sp, 0)`)
- All methods use plain `ADD_API_METHOD_N` -- no typed variants
- The manager singleton is obtained via `Helpers::getOrCreate(mc)` and stored as a `var` (ReferenceCountedObject pointer)
- `errorCallback` initialized with capacity 1 (one argument: the error string)

---

## Wrapper Struct (Method Registration)

**File:** `ScriptingApiObjects.cpp:8908-8917`

```cpp
struct Wrapper
{
    API_METHOD_WRAPPER_1(GlobalRoutingManagerReference, getCable);
    API_METHOD_WRAPPER_2(GlobalRoutingManagerReference, connectToOSC);
    API_METHOD_WRAPPER_2(GlobalRoutingManagerReference, sendOSCMessage);
    API_VOID_METHOD_WRAPPER_2(GlobalRoutingManagerReference, addOSCCallback);
    API_METHOD_WRAPPER_1(GlobalRoutingManagerReference, removeOSCCallback);
    API_VOID_METHOD_WRAPPER_3(GlobalRoutingManagerReference, setEventData);
    API_METHOD_WRAPPER_2(GlobalRoutingManagerReference, getEventData);
};
```

All use `API_METHOD_WRAPPER_N` or `API_VOID_METHOD_WRAPPER_N` -- no typed wrappers (`ADD_TYPED_API_METHOD_N`).

---

## Factory / obtainedVia

**File:** `ScriptingApi.cpp:2506-2509`

```cpp
juce::var ScriptingApi::Engine::getGlobalRoutingManager()
{
    return var(new ScriptingObjects::GlobalRoutingManagerReference(getScriptProcessor()));
}
```

Each call to `Engine.getGlobalRoutingManager()` creates a new `GlobalRoutingManagerReference` wrapper,
but they all share the same underlying `GlobalRoutingManager` singleton via `Helpers::getOrCreate()`.

---

## Core Singleton: scriptnode::routing::GlobalRoutingManager

**File:** `HISE/hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.h:55-462`

This is the actual manager object (not the scripting wrapper). It is a `ReferenceCountedObject`
stored per-MainController. Key members:

| Member | Type | Purpose |
|--------|------|---------|
| `cables` | `SlotBase::List` | All registered Cable slots |
| `signals` | `SlotBase::List` | All registered Signal slots |
| `additionalEventStorage` | `hise::AdditionalEventStorage` | Per-event-ID data storage |
| `sender` | `OSCBase::Ptr` | The active HiseOSCSender (or null) |
| `receiver` | `OSCBase::Ptr` | The active HiseOSCReceiver (or null) |
| `lastData` | `OSCConnectionData::Ptr` | Last OSC connection configuration |
| `oscErrorHandler` | `WeakErrorHandler::Ptr` | Error callback target |
| `scriptCallbackPatterns` | `Array<OSCAddressPattern>` | Patterns registered by script OSC callbacks |
| `listUpdater` | `LambdaBroadcaster<SlotType, IdList>` | Notifies UI when cable/signal lists change |
| `oscListeners` | `LambdaBroadcaster<OSCConnectionData::Ptr>` | Notifies listeners on OSC connection changes |
| `uuidManager` | `GlobalUUIDManager` | UUID management for DLL boundary |

### Helpers::getOrCreate(MainController* mc)

**File:** `GlobalRoutingManager.cpp:40-53`

```cpp
GlobalRoutingManager::Ptr newP = dynamic_cast<GlobalRoutingManager*>(mc->getGlobalRoutingManager());
if(newP == nullptr)
{
    newP = new GlobalRoutingManager();
    newP->additionalEventStorage.getBroadcaster().enableLockFreeUpdate(mc->getGlobalUIUpdater());
    mc->setGlobalRoutingManager(newP.get());
    mc->getProcessorChangeHandler().sendProcessorChangeMessage(..., RebuildModuleList, false);
}
return newP;
```

The singleton is lazily created on first access and stored on the MainController. The event
storage broadcaster is configured for lock-free updates via the global UI updater.

---

## OSC Subsystem

### OSCConnectionData

**File:** `HISE/hi_dsp_library/node_api/helpers/ParameterData.h:42-70`

Parsed from a JSON object passed to `connectToOSC()`:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Domain` | String | `"/hise_osc_receiver"` | Root OSC address prefix. Auto-prefixed with `/` if missing. Trailing `/` stripped. |
| `SourceURL` | String | `"127.0.0.1"` | Receiver bind address |
| `SourcePort` | int | `9000` | Receiver listen port |
| `TargetURL` | String | `"127.0.0.1"` | Sender target address |
| `TargetPort` | int | `-1` | Sender target port. `-1` means read-only (no sender created). |
| `Parameters` | Object | (none) | Map of cable names to range objects for OSC value conversion |

`isReadOnly` is derived: `targetPort == -1`.

The `Parameters` object maps cable IDs to range definitions. Each range is parsed via
`RangeHelpers::getDoubleRange()` and used for normalising incoming OSC values and
denormalising outgoing values.

### connectToOSC Implementation

**File:** `GlobalRoutingManager.cpp:587-619`

1. If connection data differs from `lastData`, tears down old sender/receiver
2. Creates `HiseOSCReceiver(domain, sourcePort, this)`
3. If NOT read-only, creates `HiseOSCSender(domain, targetURL, targetPort, this)` and registers all cables as OSC targets
4. Notifies `oscListeners`
5. Returns success based on receiver.ok and sender.ok

In the scripting wrapper (`ScriptingApiObjects.cpp:9008-9049`):
- If `errorFunction` is a JS function, wraps it in a `WeakCallbackHolder` and sets as error handler
- After core `connectToOSC()`, registers this reference as an `OSCReceiver::Listener`
- Rebuilds all existing callback full addresses with the new domain
- Registers callback patterns with the manager's `scriptCallbackPatterns`

**Note:** The scripting wrapper's `connectToOSC` always returns `false` (line 9048). This appears
to be a bug -- the return value from the core `m->connectToOSC(data)` is checked but the method
returns `false` unconditionally at the end. The `ok` variable is used only to decide whether to
register the listener.

### HiseOSCReceiver

**File:** `GlobalRoutingManager.cpp:368-505`

Wraps `juce::OSCReceiver`. On construction:
- Registers a format error handler that calls `GlobalRoutingManager::handleParsingError`
- Connects to the source port
- Creates an `InternalListener` that processes incoming messages

**InternalListener::oscMessageReceived:**
1. Checks message address starts with the domain URL
2. Strips domain prefix to get the cable sub-address
3. Uses `Helpers::getCableIds()` to split multi-argument messages into per-cable IDs
4. For each argument, calls `sendInternal(cableId, arg)`

**InternalListener::sendInternal:**
1. Only accepts float32, int32, bool OSC types
2. Converts to double
3. If the cable ID has a matching `inputRange` in `lastData->inputRanges`, converts to 0..1
4. Validates value is within -0.1..1.1 range (sends OSC error if not)
5. Finds matching cable (must start with '/') and calls `cable->sendValue(nullptr, value)`

### HiseOSCSender

**File:** `GlobalRoutingManager.cpp:507-584`

Wraps `juce::OSCSender`. On construction connects to target URL/port.

**OSCCableTarget (inner class):**
- A `CableTargetBase` that is added to each cable when a sender is active
- Stores an `outputRange` derived from the connection data's `Parameters` for that cable ID
- `sendValue(v)`: converts from 0..1 using outputRange, creates `OSCMessage(address, (float)v)`, sends

### Helpers::getCableIds

**File:** `GlobalRoutingManager.cpp:120-137`

For single-argument messages: returns the address portion after the domain.
For multi-argument messages: appends `[0]`, `[1]`, etc. to create per-argument cable IDs.
This allows a single OSC message with multiple args to control multiple cables.

### sendOSCMessageToOutput

**File:** `GlobalRoutingManager.cpp:692-736`

1. Gets the `HiseOSCSender`
2. Constructs `OSCAddressPattern(domain + subAddress)`
3. Converts `var` data to OSC arguments:
   - double -> float32
   - bool/int/int64 -> int32
   - string -> string
   - other -> throws "illegal var type for OSC data"
4. Handles both single values and arrays
5. Sends via `sender->send(m)`

---

## OSC Callback System (Script-Level)

**File:** `ScriptingApiObjects.h:2734-2756`, `ScriptingApiObjects.cpp:9063-9173`

### OSCCallback Inner Class

```cpp
struct OSCCallback: public ReferenceCountedObject
{
    WeakCallbackHolder callback;   // JS function (2 args)
    const String subDomain;         // sub-address registered by script
    OSCAddressPattern fullAddress;  // domain + subDomain
    var args[2];                    // pre-allocated: [0]=subDomain, [1]=value(s)
};
```

**Construction:** `callback.incRefCount()` and `callback.setHighPriority()` -- this means OSC
callbacks are not deferred, they execute on the OSC receiver thread.

**callForMessage:**
- Single-arg message: `args[1] = getVar(arg)` directly
- Multi-arg message: creates an `Array<var>` with all args
- Calls `callback.call(args, 2)` -- the callback receives (subAddress, value/array)

**getVar conversion:**
- float32 -> var(float)
- string -> var(string)
- int32 -> var(int)
- other -> var() (undefined)

### addOSCCallback

**File:** `ScriptingApiObjects.cpp:9112-9126`

1. Creates new `OSCCallback(this, oscSubAddress, callback)`
2. If manager has active OSC connection (`lastData != nullptr`), rebuilds full address and registers pattern
3. Adds to `callbacks` list

### removeOSCCallback

**File:** `ScriptingApiObjects.cpp:9095-9110`

Iterates callbacks, removes first match by `subDomain`. Returns true if found.

### oscMessageReceived (on the scripting wrapper)

**File:** `ScriptingApiObjects.cpp:8975-8993`

1. Gets address pattern from message
2. Only processes non-wildcard addresses (`!containsWildcards()`)
3. Iterates all registered callbacks, fires those whose `fullAddress` matches

### oscBundleReceived

**File:** `ScriptingApiObjects.cpp:8964-8973`

Recursively unpacks bundles into individual messages.

---

## Event Data Storage

### AdditionalEventStorage

**File:** `HISE/hi_tools/hi_tools/MiscToolClasses.h:2716-2773`

A fixed-size hash table that stores double values keyed by (eventId, slotIndex):

| Constant | Value | Description |
|----------|-------|-------------|
| `NumEventSlots` | 1024 | Number of event ID hash buckets |
| `NumDataSlots` | 16 | Number of data slots per event |

**Storage:** `std::array<std::array<std::pair<uint16, double>, NumDataSlots>, NumEventSlots>`

**Hashing:** `eventId & (NumEventSlots - 1)` -- simple bitmask, meaning event IDs map to 1024 buckets
with potential collisions. The `pair.first` stores the actual event ID to detect collisions.

**setValue(eventId, slotIndex, value, notification):**
- Writes to `data[eventId & 1023][slotIndex & 15]`
- Stores `{eventId, value}` pair (the eventId is stored to validate reads)
- Fires broadcaster with `sendNotificationSync`

**getValue(eventId, slotIndex):**
- Returns `{false, 0.0}` for eventId == 0
- Reads `data[eventId & 1023][slotIndex & 15]`
- If stored eventId matches, returns `{true, value}`
- Otherwise returns `{false, 0.0}` (hash collision or not written)

**Broadcaster:** `LambdaBroadcaster<uint16, uint8, double>` -- enabled for lock-free updates in
`getOrCreate()`. Can be used by other systems (e.g. EventDataModulator) to listen for changes.

### setEventData (scripting wrapper)

**File:** `ScriptingApiObjects.cpp:9140-9148`

```cpp
m->additionalEventStorage.setValue((uint16)eventId, (uint8)dataSlot, value, sendNotificationSync);
```

Casts eventId to uint16 and dataSlot to uint8. Always uses `sendNotificationSync`.
**Note:** Always returns `false` -- another apparent bug similar to `connectToOSC`.

### getEventData (scripting wrapper)

**File:** `ScriptingApiObjects.cpp:9150-9161`

```cpp
auto nv = m->additionalEventStorage.getValue((uint16)eventId, (uint8)dataSlot);
if(nv.first)
    return var(nv.second);
return var();  // undefined
```

Returns the stored double value if found, or `undefined` if not written or hash collision.

### Event Data Consumers

The event data storage is accessed by multiple systems:
- **EventDataModulator** (`hi_core/hi_modules/modulators/mods/EventDataModulator.h:107`) -- reads event data as modulation source
- **scriptnode routing nodes** (`hi_dsp_library/dsp_nodes/RoutingNodes.h:447,576`) -- access via `TempoSyncer::additionalEventStorage`
- **ComplexGroupManager** (`hi_core/hi_sampler/sampler/ComplexGroupManager.cpp:869`) -- uses for sample group routing

---

## Destructor

**File:** `ScriptingApiObjects.cpp:8937-8947`

```cpp
~GlobalRoutingManagerReference()
{
    if (auto m = dynamic_cast<GlobalRoutingManager*>(manager.getObject()))
    {
        if (auto r = dynamic_cast<OSCReceiver*>(m->receiver.get()))
            r->removeListener(this);

        for (auto c : callbacks)
            m->scriptCallbackPatterns.removeAllInstancesOf(c->fullAddress);
    }
}
```

Cleans up: removes itself as OSC listener and unregisters all callback patterns.

---

## Private Members

| Member | Type | Purpose |
|--------|------|---------|
| `errorCallback` | `WeakCallbackHolder` | JS error function for OSC errors (1 arg) |
| `callbacks` | `OSCCallback::List` (`ReferenceCountedArray`) | Registered OSC sub-address callbacks |
| `manager` | `var` | Reference to the core `GlobalRoutingManager` singleton |

---

## Threading Considerations

- OSC callbacks execute on the OSC receiver thread (set via `setHighPriority()` on the WeakCallbackHolder)
- `setEventData` uses `sendNotificationSync` -- the broadcaster fires synchronously on the calling thread
- The event storage is a flat array with no locking -- relies on atomic-width writes for the double values
- Cable values flow through the Cable::sendValue mechanism documented in the GlobalCable exploration

---

## Signal Slots (Not Exposed to Scripting API)

The core `GlobalRoutingManager` also manages `Signal` slots (`GlobalRoutingManager.h:307-339`) for
audio signal routing between scriptnode nodes. These are NOT exposed through the scripting API --
only the cable factory (`getCable`) and OSC/event-data methods are available to scripts.

---

## OSC Address Pattern: Cable ID Convention

For OSC integration, cable IDs that start with `/` are treated as OSC-addressable. When an OSC
receiver gets a message, it strips the domain prefix and matches the remaining address against
cable IDs. Only cables whose IDs start with `/` participate in OSC routing.

For multi-argument OSC messages, arguments are mapped to cables with bracketed indices:
`/param[0]`, `/param[1]`, etc.

---

## Debug UI

`createPopupComponent()` delegates to `GlobalRoutingManager::Helpers::createDebugViewer(mc)`.
This is a `USE_BACKEND`-only feature that provides a visual overview of all cables and signals.
