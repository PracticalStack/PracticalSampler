# GlobalCable -- Class Analysis

## Brief
Named data bus for routing normalised values and arbitrary data between script processors and modules.

## Purpose
GlobalCable provides a named, project-wide communication channel that carries both normalised double values (0..1) and arbitrary serialised data (JSON, strings, buffers). Multiple script processors, scriptnode nodes, UI components, macro controls, and module parameters can connect to the same cable by name. Each scripting reference maintains its own local input range that maps user-facing values to/from the internal normalised space. Value callbacks can run synchronously (audio-thread safe with realtime-safe functions) or asynchronously (polled on the UI thread via PooledUIUpdater). Data callbacks are always asynchronous.

## Details

### Architecture

GlobalCable is a scripting wrapper (`GlobalCableReference`) around `GlobalRoutingManager::Cable`. The routing manager is a singleton per `MainController`, lazily created on first access. Cables are identified by string name and created on demand -- requesting a cable by name that does not yet exist creates it.

### Dual Channel Design

Each cable carries two independent channels:

| Channel | Storage | Dispatch | API |
|---------|---------|----------|-----|
| Value | `double lastValue` (0..1, clamped) | Immediate to all targets | `setValue`, `setValueNormalised`, `getValue`, `getValueNormalised`, `registerCallback` |
| Data | `MemoryBlock lastData` (serialised `var`) | Immediate to all targets | `sendData`, `registerDataCallback` |

Value callbacks and data callbacks are separate: a `Callback` ignores data, a `DataCallback` ignores values.

### Input Range

Each `GlobalCableReference` has a local `InvertableParameterRange inputRange` (default: 0..1 identity). The range affects:
- `setValue()` -- converts input to 0..1 before sending
- `getValue()` -- converts 0..1 back to the input range
- Registered value callbacks -- receive values converted through the range

The range does NOT affect `setValueNormalised()`, `getValueNormalised()`, or `sendData()`.

Note: Without a range configured, the default 0..1 identity range applies, making `setValue` and `setValueNormalised` behave identically. This is valid if you intend to work in normalised space.

### Callback Dispatch Modes

| Mode | Thread | Mechanism | Requirement |
|------|--------|-----------|-------------|
| Synchronous | Calling thread (may be audio) | `WeakCallbackHolder::callSync()` direct | Function must be realtime-safe (`isRealtimeSafe()`) |
| Asynchronous | UI thread | `ModValue` + `PooledUIUpdater::SimpleTimer` polling | None -- coalesces rapid changes |

Async callbacks use `ModValue::setModValueIfChanged()` / `getChangedValue()`, which means only the latest value is delivered and intermediate values are dropped.

### Target System

The underlying `Cable` maintains an `Array<WeakReference<CableTargetBase>>` target list. Targets include:
- **DummyTarget** -- registered by each script reference for debug UI visibility (no-op `sendValue`)
- **Callback** -- registered by `registerCallback()` for script value callbacks
- **DataCallback** -- registered by `registerDataCallback()` for script data callbacks
- **MacroCableTarget** -- registered by `connectToMacroControl()`, scales 0..1 -> 0..127
- **ProcessorParameterTarget** -- registered by `connectToModuleParameter()`, applies target range and optional smoothing
- **GlobalCableNode** -- scriptnode `routing.global_cable` node
- **ScriptComponent::GlobalCableConnection** -- UI components with `processorId` set to `"GlobalCable"`

On `addTarget`, the new target immediately receives the current `lastValue` and any stored `lastData`.

### Clearing Connections

- `connectToMacroControl(-1, true, false)` -- removes all macro targets
- `connectToModuleParameter("", -1, {})` -- removes all module parameter targets
- `connectToModuleParameter("ProcessorId", -1, {})` -- removes all targets for that processor

## obtainedVia
`Engine.getGlobalRoutingManager().getCable(cableId)`

## minimalObjectToken
cable

## Constants
(None)

## Dynamic Constants
(None)

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using a non-realtime-safe function as a synchronous callback | Use `inline function` or pass `AsyncNotification` | Synchronous callbacks run on the calling thread which may be the audio thread. Non-realtime-safe functions are silently rejected (callback never fires). |
| Calling `sendData()` from the audio thread | Move data sending to a timer or async context | `sendData()` allocates a `MemoryOutputStream` on the heap, which is not audio-thread safe. |

## codeExample
```javascript
// Get a global cable and register a callback
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("MyCable");

cable.setRange(0.0, 100.0);

inline function onCableValue(value)
{
    Console.print("Cable value: " + value);
};

cable.registerCallback(onCableValue, AsyncNotification);
cable.setValue(50.0);
```

## Alternatives
Broadcaster -- for scripting-only pub/sub without the global routing infrastructure. GlobalRoutingManager -- parent object that also manages OSC routing and global signals.

## Related Preprocessors
None.

## Diagrams

### cable-dispatch
- **Brief:** Cable Value Dispatch Flow
- **Type:** topology
- **Description:** A GlobalCable sits at the center of a hub-and-spoke topology. Sources (script setValue/setValueNormalised, scriptnode global_cable node, ScriptComponent with processorId="GlobalCable", GlobalModulatorContainer modulators) send normalised values into the cable. The cable stores lastValue (clamped 0..1) and fans out to all registered targets: sync/async script Callbacks (with inputRange conversion), DataCallbacks (data channel only), MacroCableTargets (0..1 -> 0..127), ProcessorParameterTargets (with target range + smoothing), and scriptnode global_cable nodes. The source that sent the value is skipped during fan-out to prevent feedback loops.

### callback-threading
- **Brief:** Sync vs Async Callback Threading
- **Type:** timing
- **Description:** When a value arrives at a synchronous Callback, callSync() executes the script function immediately on the calling thread (which may be the audio thread). When a value arrives at an asynchronous Callback, ModValue stores the latest value atomically, and the PooledUIUpdater SimpleTimer polls on the UI thread at the display refresh rate, delivering only the most recent value (intermediate values are coalesced/dropped).
