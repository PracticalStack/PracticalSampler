# Broadcaster -- Raw Context Exploration (Step A1)

## Resources Consulted

- `resources/survey/class_survey.md` -- prerequisite table, enrichment group (event)
- `resources/survey/class_survey_data.json` -- Broadcaster entry: createdBy, seeAlso, callbackDensity 0.84
- No prerequisite Readme needed (Broadcaster has no conceptual prerequisite in the table)
- No existing base class exploration applicable (not a component class)

## Source Files

| File | Role |
|------|------|
| `hi_scripting/scripting/api/ScriptBroadcaster.h` | Primary header: class declaration, all nested types |
| `hi_scripting/scripting/api/ScriptBroadcaster.cpp` | Full implementation: 5146 lines |
| `hi_scripting/scripting/api/ScriptingApi.cpp:2460` | Engine.createBroadcaster factory |
| `hi_scripting/scripting/api/ScriptingBaseObjects.h:593` | AssignableDotObject interface |
| `hi_scripting/scripting/api/ScriptingBaseObjects.h:253` | WeakCallbackHolder::CallableObject interface |
| `hi_scripting/scripting/engine/JavascriptEngineStatements.cpp:91` | ScopedBypasser language integration |
| `hi_scripting/scripting/engine/JavascriptEngineParser.cpp:745` | Parser integration for bypass() |
| `hi_scripting/scripting/components/ScriptBroadcasterMap.h` | BroadcasterMap visualization component |
| `hi_core/hi_core/MiscComponents.h:38` | MouseCallbackComponent::CallbackLevel enum |
| `hi_core/hi_core/MiscComponents.cpp:48` | getCallbackLevels() string table |
| `hi_core/hi_core/MainController.h` | specBroadcaster, realtimeBroadcaster (LambdaBroadcaster) |

---

## 1. Class Declaration and Inheritance Chain

```cpp
struct ScriptBroadcaster : public ConstScriptingObject,
                           public WeakCallbackHolder::CallableObject,
                           public AssignableDotObject,
                           private Timer
```

**ConstScriptingObject** -- standard HISE scripting API base class. Provides `getScriptProcessor()`, `reportScriptError()`, `addConstant()`, `ADD_API_METHOD_N` macro support, debug information.

**WeakCallbackHolder::CallableObject** -- interface that allows HiseScript to call the object like a function. The `call()` override is what gets invoked when the script writes `bc(arg1, arg2)` or uses dot-assignment syntax that triggers a send. Key methods: `call(engine, args, returnValue)`, `isRealtimeSafe()`, `getNumArguments()`, `getCallId()`.

**AssignableDotObject** -- enables dot-notation property assignment and reading on the scripting side. When script writes `bc.value = 42`, the engine calls `assign("value", 42)`. When reading `bc.value`, it calls `getDotProperty("value")`. The argument names from the broadcaster's `args` definition become the valid dot properties.

**private Timer** -- used internally for `sendMessageWithDelay()` deferred sends.

The object name returned to the scripting engine is `"Broadcaster"` (see `getObjectName()`).

---

## 2. Internal Class Hierarchies

The Broadcaster class has a deep nested type system with two parallel hierarchies:

### ItemBase (common base for all items)

```
ItemBase
  |-- metadata: Metadata
  |-- profileIndex: int
  |-- virtual getItemId() -> Identifier
  |-- virtual createChildArray() -> Array<var>  (for debug visualization)
  |-- virtual registerSpecialBodyItems(factory)  (for BroadcasterMap UI)
  |
  |-- TargetBase (listener/callback targets -- things that receive messages)
  |     |-- obj: var (the object passed as first arg to addListener)
  |     |-- enabled: bool (can be toggled in debug UI)
  |     |-- location: DebugableObjectBase::Location
  |     |-- virtual callSync(args) -> Result
  |     |
  |     |-- ScriptTarget -- general callback listener (addListener)
  |     |-- DelayedItem -- delayed callback listener (addDelayedListener)
  |     |-- ComponentPropertyItem -- sets component properties (addComponentPropertyListener)
  |     |-- ComponentValueItem -- sets component values (addComponentValueListener)
  |     |-- ComponentRefreshItem -- triggers repaint/changed etc (addComponentRefreshListener)
  |     |-- ModuleParameterSyncer -- syncs module parameter values (addModuleParameterSyncer)
  |     |-- OtherBroadcasterTarget -- forwards to another broadcaster (attachToOtherBroadcaster)
  |
  |-- ListenerBase (source/attachment items -- things that produce messages)
        |-- virtual getNumInitialCalls() -> int
        |-- virtual getInitialArgs(callIndex) -> Array<var>
        |-- virtual callItem(TargetBase*) -> Result
        |
        |-- ComponentPropertyListener -- watches component property changes
        |-- ComponentValueListener -- watches component value changes
        |-- ComponentVisibilityListener -- watches component visibility
        |-- InterfaceSizeListener -- watches interface resize
        |-- MouseEventListener -- watches mouse events on components
        |-- ContextMenuListener -- provides context menu on components
        |-- ModuleParameterListener -- watches module parameter changes
        |-- RadioGroupListener -- watches radio button group selection
        |-- EqListener -- watches EQ band events
        |-- ComplexDataListener -- watches complex data changes
        |-- NonRealtimeSource -- watches realtime/non-realtime changes
        |-- ProcessingSpecSource -- watches samplerate/blocksize changes
        |-- SamplemapListener -- watches samplemap events
        |-- RoutingMatrixListener -- watches routing matrix changes
        |-- OtherBroadcasterListener -- source for chained broadcasters
        |-- DebugableObjectListener -- source for script function calls
        |-- ScriptCallListener -- tracks script call sites (debug only)
```

### Key architectural pattern

The broadcaster maintains two OwnedArrays:

```cpp
OwnedArray<ListenerBase> attachedListeners;  // sources
OwnedArray<TargetBase> items;                // targets
```

**Sources** (ListenerBase) detect changes and call `sendMessageInternal()` / `sendAsyncMessage()` on the broadcaster. **Targets** (TargetBase) receive messages via `callSync()` when `sendInternal()` iterates `items`.

The flow is: External event -> ListenerBase internal listener -> `broadcaster.sendAsyncMessage(args)` -> `sendMessageInternal` -> `sendInternal` -> iterates `items` calling `target.callSyncWithProfile(args)`.

### Priority sorting

Items (targets) are sorted by metadata priority using `ItemBase::PrioritySorter`:
```cpp
// Higher priority values execute first (descending sort)
int PrioritySorter::compareElements(ItemBase* m1, ItemBase* m2)
{
    if (m1->metadata.priority > m2->metadata.priority) return -1;
    if (m1->metadata.priority < m2->metadata.priority) return 1;
    return 0;
}
```

---

## 3. Metadata System

The `Metadata` struct is used for identifying both sources and targets.

### Metadata construction from script

Metadata can be:
1. **A string** -- sets `id` to the string, `c` to grey. Empty strings fail if `mustBeValid` is true.
2. **A JSON object** -- must have at least `"id"`. Optional: `"comment"`, `"colour"` (int or -1 for auto), `"priority"` (int), `"tags"` (array of strings), `"visible"` (bool, defaults to true).

```cpp
struct Metadata
{
    Result r;
    String comment;
    Identifier id;
    int64 hash = 0;      // hashCode64 of id string
    Colour c;
    int priority = 0;
    Array<Identifier> tags;
    bool visible;         // defaults to true
};
```

Special colour value: `-1` generates an auto-colour from the hash: `Colour((uint32)hash).withBrightness(0.7f).withSaturation(0.6f)`.

### Metadata matching

Listeners and targets are identified by metadata for removal:
```cpp
bool removeListener(var objectToRemove)
{
    for (auto i : items)
        if (i->metadata == objectToRemove)
            { items.removeObject(i); return true; }
    return false;
}
```

This means the `removeListener` parameter is matched against the metadata hash, not the object reference.

---

## 4. Constructor and Initialization

```cpp
ScriptBroadcaster(ProcessorWithScriptingContent* p, const var& md)
```

The constructor accepts the `defaultValues` parameter from `Engine.createBroadcaster()`. Four formats:

### Format 1: JSON with id and args
```javascript
Engine.createBroadcaster({ id: "myBc", args: ["component", "value"] });
```
Creates a broadcaster with named arguments as identifiers, all default values set to undefined. The metadata is extracted from the outer JSON.

### Format 2: JSON with named properties (legacy format)
```javascript
Engine.createBroadcaster({ component: 0, value: 0 });
```
Property names become argument IDs, property values become default values.

### Format 3: Plain array of strings
```javascript
Engine.createBroadcaster(["component", "value"]);
```
String elements become argument IDs, default values are all undefined.

### Format 4: Single value (simplest)
```javascript
Engine.createBroadcaster(0);
```
Creates a 1-argument broadcaster with no argument name and default value of 0.

The constructor also:
- Registers with `JavascriptProcessor::registerCallableObject(this)` to enable function-call syntax
- Sets `wantsCurrentLocation(true)` for debug tracking
- Sets up `ProfileCollection` for Perfetto profiling

### API method registrations

40 methods registered via `ADD_API_METHOD_N` macros (all untyped -- no forced parameter types):

**Listener management (add targets):**
- addListener(3), addDelayedListener(4), addComponentPropertyListener(4), addComponentValueListener(3), addComponentRefreshListener(3), addModuleParameterSyncer(3)

**Listener removal:**
- removeListener(1), removeSource(1), removeAllListeners(0), removeAllSources(0)

**Message sending:**
- sendMessage(2) [deprecated], sendSyncMessage(1), sendAsyncMessage(1), sendMessageWithDelay(2)

**Source attachment:**
- attachToComponentProperties(3), attachToComponentValue(2), attachToComponentVisibility(2), attachToInterfaceSize(1), attachToComponentMouseEvents(3), attachToContextMenu(5), attachToComplexData(4), attachToEqEvents(3), attachToModuleParameter(3), attachToRadioGroup(2), attachToOtherBroadcaster(4), attachToRoutingMatrix(2), attachToProcessingSpecs(1), attachToNonRealtimeChange(1), attachToSampleMap(3)

**Configuration:**
- setReplaceThisReference(1), setEnableQueue(1), setRealtimeMode(1), setForceSynchronousExecution(1), setSendMessageForUndefinedArgs(1)

**State management:**
- reset(0), resendLastMessage(1), setBypassed(3), isBypassed(0), callWithDelay(3), refreshContextMenuState(0)

---

## 5. Dot-Assignment Operator (AssignableDotObject)

The `assign()` method enables writing individual arguments:

```javascript
bc.value = 42;  // calls assign("value", 42)
```

Implementation:
```cpp
bool ScriptBroadcaster::assign(const Identifier& id, const var& newValue)
{
    auto idx = argumentIds.indexOf(id);
    if (idx == -1) reportScriptError("...");

    if (lastValues[idx] != newValue || enableQueue)
    {
        lastValues.set(idx, newValue);
        lastResult = sendInternal(lastValues);  // synchronous send
    }
    return true;
}
```

Key insight: **Dot-assignment always sends synchronously** and sends the entire `lastValues` array. Only the changed argument is updated before sending. If the same value is assigned, the message is suppressed (unless queue is enabled).

Reading: `var x = bc.value;` calls `getDotProperty("value")` which returns `lastValues[idx]`.

---

## 6. Function Call Syntax (CallableObject)

When a broadcaster is called like a function, `call()` is invoked:

```javascript
bc(componentRef, 42);
```

The `call()` method has special handling for RadioGroupListener:
- If attached to a radio group, it interprets args as (clickedButton, isOn) and dispatches via `sendAsyncMessage(idx)`
- Otherwise, it validates arg count matches `defaultValues.size()` and calls `sendMessageInternal()`

When called from a function call context: `shouldBeSync = attachedListeners.isEmpty()` -- if no sources are attached, it sends synchronously; if sources are attached, it sends asynchronously.

---

## 7. Send Message Internals and Threading

### sendMessageInternal(args, isSync)

This is the core dispatch method. Flow:

1. If `forceSync` is true, forces synchronous mode
2. **Backend safety check**: If sync + audio thread + not realtimeSafe, throws error
3. Validates argument count matches defaultValues.size()
4. **Realtime-safe fast path**: If sync + realtimeSafe, updates lastValues directly (no lock), calls `sendInternal()`, returns
5. **Normal path**: Compares new values to last values. If nothing changed (and queue disabled and forceSend is false), returns without sending
6. Acquires `lastValueLock` (SimpleReadWriteLock) to update lastValues
7. If bypassed, returns silently
8. **Sync path**: Calls `sendInternal(lastValues)` directly
9. **Async path**: Posts a `HiPriorityCallbackExecution` job to `JavascriptThreadPool`. Uses `asyncPending` atomic to coalesce rapid async sends (unless queue is enabled). If queue is enabled, captures lastValues snapshot for the queued job.

### sendInternal(args)

Iterates `items` (targets) and calls `callSyncWithProfile()` on each:

- **Realtime-safe mode**: Iterates items directly, calling callSync() without copying args
- **Normal mode**: Takes a read lock on lastValueLock, copies args per-item, calls callSyncWithProfile()
- After iterating targets, checks for RadioGroupListener and ContextMenuListener special post-processing
- Returns Result::ok() unless a target callback fails

### Change detection

```cpp
bool somethingChanged = false;
for (int i = 0; i < defaultValues.size(); i++)
{
    auto v = BroadcasterHelpers::getArg(args, i);
    somethingChanged |= lastValues[i] != v;
}

if (somethingChanged || enableQueue || forceSend) { ... }
```

The `cancelIfSame` member exists but is not exposed via API (always true). The change detection uses `var::operator!=` which does value comparison for primitives and reference comparison for objects.

### Undefined argument handling

By default, `sendInternal()` returns early (no callbacks) if any argument is undefined/void:
```cpp
for (int i = 0; i < defaultValues.size(); i++)
{
    auto v = args[i];
    if (v.isUndefined() || v.isVoid())
        return Result::ok();
}
```

This can be overridden with `setSendMessageForUndefinedArgs(true)`.

---

## 8. Queue Mode

When `enableQueue` is true:
- **All messages are dispatched**, even if the value has not changed
- **All async messages go through**, even if one is already pending (the asyncPending gate is bypassed)
- Each queued message captures its own lastValues snapshot (copy) rather than referencing the shared lastValues

Queue mode is **automatically enabled** by certain attach methods:
- `attachToModuleParameter` -- because multiple parameter changes may arrive before the JS thread processes
- `attachToRoutingMatrix` -- same reason
- `attachToContextMenu` -- explicitly set to true
- `attachToComplexData` -- enabled when multiple processors or indices
- `attachToSampleMap` -- enabled when multiple samplers, event types, or SampleChanged events

Queue mode can also be manually set via `setEnableQueue(true)`.

---

## 9. Realtime Mode

When `realtimeSafe` is true:
- `sendInternal()` skips the SimpleReadWriteLock read lock and iterates items directly
- `sendMessageInternal()` skips the write lock, value comparison, and async path -- goes straight to sendInternal()
- `isRealtimeSafe()` returns true, which is checked by the runtime safety analysis system

This mode is **automatically enabled** by `attachToNonRealtimeChange()`.

The backend performs a safety check: if a synchronous message is sent from the audio thread without realtime mode enabled, a script error is thrown.

When `addListener()` is called on a realtime-safe broadcaster, the callback function is checked:
- In backend: `RealtimeSafetyInfo::check()` validates the callback
- In frontend: Only inline functions are accepted (checked via `isRealtimeSafe()`)

---

## 10. Bypass System

```cpp
void setBypassed(bool shouldBeBypassed, bool sendMessageIfEnabled, bool async)
{
    if (shouldBeBypassed != bypassed)
    {
        bypassed = shouldBeBypassed;
        if (!bypassed && sendMessageIfEnabled)
            resendLastMessage(async);
    }
}
```

When bypassed:
- `sendMessageInternal()` stores new lastValues but returns before dispatching
- `initItem()` skips initialization when bypassed
- `checkMetadataAndCallWithInitValues()` returns early when bypassed

### HiseScript bypass Statement Integration

The parser recognizes a special `bypass(broadcaster, sendOnResume)` scoped statement:

```javascript
bypass(myBroadcaster, true)
{
    // broadcaster is bypassed during this block
    // On block exit, bypass state is restored
    // If sendOnResume is true and was not bypassed before, resends last message
}
```

Implementation via ScopedBypasser:
- `perform()`: Saves current bypass state, sets bypassed to true
- `cleanup()`: Restores previous bypass state; if was not bypassed before, sends message if sendOnResume is true

---

## 11. Source Attachment Details

### throwIfAlreadyConnected()

Currently a no-op (the check is #if 0'd out). Multiple sources can be attached to a single broadcaster, though historically this was not allowed.

### Argument count requirements per attach method

Each attach method validates `defaultValues.size()`:

| Method | Required Args | Arg Semantics |
|--------|-------------|---------------|
| attachToComponentProperties | 3 | (component, propertyId, value) |
| attachToComponentValue | 2 | (component, value) |
| attachToComponentVisibility | 2 | (id, isVisible) |
| attachToInterfaceSize | 2 | (width, height) |
| attachToComponentMouseEvents | 2 | (component, event) |
| attachToContextMenu | 2 | (component, menuItemIndex) |
| attachToModuleParameter | 3 | (processorId, parameterId, value) |
| attachToRadioGroup | 1 | (selectedIndex) |
| attachToComplexData | 3 | (processorId, index, value) |
| attachToEqEvents | 2 | (eventType, value) |
| attachToRoutingMatrix | 2 | (processorId, matrix) |
| attachToProcessingSpecs | 2 | (sampleRate, blockSize) |
| attachToNonRealtimeChange | 1 | (isNonRealtime) |
| attachToSampleMap | 3 | (eventType, samplerId, data) |
| attachToOtherBroadcaster | -- | (inherits source broadcaster arg count) |

### Initial value dispatch pattern

When a source is attached via `checkMetadataAndCallWithInitValues()`:

```cpp
if (auto l = dynamic_cast<ListenerBase*>(i))
{
    int numInitArgs = l->getNumInitialCalls();
    for (int j = 0; j < numInitArgs; j++)
    {
        lastValues = l->getInitialArgs(j);
        for (auto target : items)
            target->callSyncWithProfile(*this, lastValues);
    }
}
```

This means: when a source is attached, it immediately fires all its initial values to all already-registered targets. This ensures UI state is synchronized at init time.

Similarly, when a new target is added via `initItem()`:
```cpp
if (!attachedListeners.isEmpty() && !isBypassed())
{
    for (auto attachedListener : attachedListeners)
        attachedListener->callItemWithProfile(*this, ni);
}
```

### Initial calls per listener type

| ListenerBase Type | getNumInitialCalls() |
|---|---|
| ComponentPropertyListener | numComponents * numProperties |
| ComponentValueListener | items.size() (one per component) |
| ComponentVisibilityListener | items.size() |
| InterfaceSizeListener | 1 |
| MouseEventListener | 0 (non-persistent events) |
| ContextMenuListener | 0 |
| ModuleParameterListener | sum of (parameterIndexes.size() + specialId) per listener |
| RadioGroupListener | 1 |
| EqListener | 0 |
| ComplexDataListener | items.size() |
| NonRealtimeSource | 0 |
| ProcessingSpecSource | 0 |
| SamplemapListener | count of registered SampleMapChanged + SamplesAddedOrRemoved events per item |
| RoutingMatrixListener | listeners.size() |
| OtherBroadcasterListener | sources.size() |
| DebugableObjectListener | 0 |
| ScriptCallListener | 0 |


---

## 12. Enum/Constant Behavioral Tracing

### ComponentRefreshItem::RefreshType

Defined in `ScriptBroadcaster.h:421`. Parsed from string in the constructor at `ScriptBroadcaster.cpp:2973`:

| Enum Value | String Match | Behavioral Consequence |
|---|---|---|
| `repaint` | `"repaint"` | Calls `sc->sendRepaintMessage()` on each target component |
| `changed` | `"changed"` | Calls `sc->changed()` on each target component |
| `updateValueFromProcessorConnection` | `"updateValueFromProcessorConnection"` | Calls `sc->updateValueFromProcessorConnection()` - refreshes the value from its connected processor parameter |
| `loseFocus` | `"loseFocus"` | Calls `sc->loseFocus()` on each target component |
| `resetValueToDefault` | `"resetValueToDefault"` | Calls `sc->resetValueToDefault()` on each target component |
| `numRefreshTypes` | (sentinel) | If the string didn't match any above, this remains set and triggers `reportScriptError("Unknown refresh mode: " + refreshType)` at `ScriptBroadcaster.cpp:3546` |

The `callSync()` method ignores its `args` parameter entirely (declared as `const Array<var>&` but unnamed). It iterates `obj` (the array of target components) and calls the appropriate method on each `ScriptComponent*`. Each component has an associated `RefCountedTime` slot whose `lastTime` is updated to `Time::getMillisecondCounter()` for the debug blink visualization.

### MouseCallbackComponent::CallbackLevel

Defined in `MiscComponents.h:102`:

```cpp
enum class CallbackLevel
{
    NoCallbacks = 0,
    PopupMenuOnly,
    ClicksOnly,
    ClicksAndEnter,
    Drag,
    AllCallbacks
};
```

The string table from `getCallbackLevels(false)` in `MiscComponents.cpp:48`:

| Enum Value | Display String |
|---|---|
| `NoCallbacks` | `"No Callbacks"` |
| `PopupMenuOnly` | `"Context Menu"` |
| `ClicksOnly` | `"Clicks Only"` |
| `ClicksAndEnter` | `"Clicks & Hover"` |
| `Drag` | `"Clicks, Hover & Dragging"` |
| `AllCallbacks` | `"All Callbacks"` |

There is also an `Identifier`-based accessor `getCallbackLevelAsIdentifier()` in `MiscComponents.h:112` that returns identifiers matching the enum names exactly (e.g., `"NoCallbacks"`, `"PopupMenuOnly"`).

The file callback variant `getCallbackLevels(true)` returns a different set: `"No Callbacks"`, `"Drop Only"`, `"Drop & Hover"`, `"All Callbacks"`.

Used by `attachToComponentMouseEvents()` which accepts the `callbackLevel` parameter and passes it to `MouseEventListener`.

### SamplemapListener::EventTypes

Defined in `ScriptBroadcaster.h:611`:

```cpp
enum class EventTypes
{
    SampleMapChanged,
    SamplesAddedOrRemoved,
    SampleChanged,
    numEventTypes
};
```

The `Event` constructor at `ScriptBroadcaster.h:623` parses from var:
- **String `"SampleMapChanged"`** - sets `type = EventTypes::SampleMapChanged`
- **String `"SamplesAddedOrRemoved"`** - sets `type = EventTypes::SamplesAddedOrRemoved`
- **Integer** - interpreted as a `SampleIds` index. Sets `type = EventTypes::SampleChanged` and `id = SampleIds::Helpers::getAllIds()[idx]`

Note: `SampleChanged` cannot be specified by string - it requires an integer index into the SampleIds list. This is because `SampleChanged` events are per-property, identified by the sample property ID.

Queue mode is auto-enabled when: `processors.size() > 1 || eventTypes.size() > 1 || eventTypes[0].type == SampleChanged` (line 4281).

### attachToComplexData dataTypeAndEvent format

At `ScriptBroadcaster.cpp:4288`:

```cpp
const String dataType = dataTypeAndEvent.upToFirstOccurrenceOf(".", false, false);
const String eventType = dataTypeAndEvent.fromFirstOccurrenceOf(".", false, false);
```

Format: `"DataType.EventType"` (e.g., `"AudioFile.Content"`)

The `dataType` part is matched against `ExternalData::getDataTypeName(t, false)` for each data type (AudioFile, Table, SliderPack, FilterCoefficients, DisplayBuffer, etc.).

The `eventType` determines the listener mode:
- `"Display"` or `"DisplayIndex"` - creates a display listener (`isDisplay = true`)
- `"Content"` - creates a content change listener (`isDisplay = false`)

If either part is empty, a script error is thrown: `"dataTypeAndEvent must be formatted like 'AudioFile.Content'"`.

Queue mode auto-enables when `processors.size() > 1 || indexListArray.size() > 1`.

### attachToEqEvents valid types

At `ScriptBroadcaster.cpp:4414`:

```cpp
StringArray legitEventTypes = { "BandAdded", "BandRemoved", "BandSelected", "FFTEnabled" };
```

If the `events` parameter is an empty string or empty array, all four event types are subscribed (the `eventTypes` array gets swapped with `legitEventTypes` at line 4433). Each event type string is validated against the `legitEventTypes` list; invalid types trigger `reportScriptError`.

### attachToModuleParameter special IDs

At `ScriptBroadcaster.cpp:4073-4110`, certain parameter name strings are treated as special identifiers rather than normal parameter lookups:

| Special ID | Condition | Behavior |
|---|---|---|
| `"Bypassed"` | Always valid | Registers as a `BypassListener` on the processor. The bypass state fires with `bypassIdAsVar` as the parameter name argument |
| `"Enabled"` | Always valid | Same as `"Bypassed"` - both result in `specialId = Identifier(pName)` and register a bypass listener |
| `"Intensity"` | Only if processor is a `Modulator*` | Registers on `mod->intensityBroadcaster` via `addListener(*this, intensityChanged, true)` instead of attribute listening |

These special IDs are detected by string comparison before the normal `getParameterIndexForIdentifier()` lookup. If a parameter array contains both special IDs and normal parameters, the special IDs are extracted via `continue` and the normal ones are resolved to integer indices.

### attachToContextMenu stateFunction callback types

At `ScriptBroadcaster.cpp:2279-2310`, the `stateFunction` is called with two arguments `(type, index)`:

| Type String | Method | Default Return | Purpose |
|---|---|---|---|
| `"active"` | `itemIsTicked(index)` | `var(false)` | Whether the menu item shows a tick/checkmark |
| `"enabled"` | `itemIsEnabled(index)` | `var(false)` | Whether the menu item is interactable (not greyed out) |
| `"text"` | `getDynamicItemText(index)` | `var("")` | Dynamic text override for the menu item |

These cached values are computed at initialization and refreshed after each menu selection (in `sendInternal` post-processing at line 4679-4680) or when `refreshContextMenuState()` is called explicitly.

---

## 13. Helper Classes

### BroadcasterHelpers

Defined at `ScriptBroadcaster.cpp:93` as a `struct` with static utility methods:

```cpp
struct BroadcasterHelpers
{
    static int getNumArgs(const var& defaultValue);
    static var getArg(const var& v, int idx);
    static var getListOrFirstElement(const var& l);
    static var getIdListAsVar(const Array<Identifier>& propertyIds);
    static Array<Identifier> getIdListFromVar(const var& propertyIds);
    static bool isValidArg(var valueOrList, int index = -1);
    static Identifier getIllegalProperty(Array<ScriptComponent*>& componentList, const Array<Identifier>& propertyIds);
    static Array<ScriptComponent*> getComponentsFromVar(ProcessorWithScriptingContent* p, var componentIds);
    static void callForEachIfArray(const var& obj, const std::function<bool(const var& d)>& f);
};
```

Key functions:

- **`getNumArgs(defaultValue)`** - returns `defaultValue.size()` for arrays, `obj->getProperties().size()` for dynamic objects, `1` for everything else. Used during construction to determine argument count.

- **`getArg(v, idx)`** - extracts argument from var: if array, returns `v[idx]`; otherwise asserts `idx == 0` and returns `v`. Central to how single-value vs array broadcasters work.

- **`getListOrFirstElement(l)`** - if `l` is an array of size 1, unwraps to the single element. Used to normalize component lists so single components don't get wrapped unnecessarily.

- **`getComponentsFromVar(p, componentIds)`** - resolves component references from strings (by name lookup via `content->getComponentWithName()`) or objects (by `dynamic_cast<ScriptComponent*>()`). Handles both single values and arrays. Removes null entries.

- **`callForEachIfArray(obj, f)`** - iterates over `obj` as an array (with early break if `f` returns false), or calls `f` once with the whole value if not an array. Used extensively by target items to handle both single-component and multi-component targets uniformly.

### isPrimitiveArray

Static method on `ScriptBroadcaster` at `ScriptBroadcaster.cpp:4563`:

```cpp
bool ScriptBroadcaster::isPrimitiveArray(const var& obj)
{
    if (obj.isArray())
    {
        bool isPrimitive = true;
        for (auto& v : *obj.getArray())
        {
            isPrimitive &= (!v.isObject() && !v.isArray());
            if (!isPrimitive) break;
        }
        return isPrimitive;
    }
    return false;
}
```

Returns `true` if `obj` is an array where every element is a non-object, non-array value (numbers, strings, bools). Used by `ScriptTarget::createChildArray()` (line 873) to decide whether to treat an array argument as a single opaque value (primitive array treated as one item) or as multiple child items (array of objects gets expanded). Also used in `ScriptingApiContent.cpp:10466` for component property handling.

---

## 14. Target Types Detail

### ScriptTarget

The general-purpose listener target created by `addListener()`.

**Constructor** (`ScriptBroadcaster.cpp:849`):
```cpp
ScriptTarget(ScriptBroadcaster* sb, int numArgs, const var& obj_, const var& f, const var& metadata_):
    TargetBase(obj_, f, metadata_),
    callback(sb->getScriptProcessor(), sb, f, numArgs)
{
    metadata.attachCommentFromCallableObject(f);
    callback.incRefCount();
    callback.addAsSource(sb, metadata.id.toString());
}
```

- Wraps the callback function in a `WeakCallbackHolder` with `numArgs` matching the broadcaster's argument count
- Marks callback as high-priority (via `WeakCallbackHolder` constructor defaults)
- The `this` replacement: when `replaceThisReference` is true on the broadcaster (default), the `obj` passed to `addListener` replaces the `this` reference in the callback function call. This happens through `var::NativeFunctionArgs(obj, args.getRawDataPointer(), args.size())` in `callSync()` at line 898.

**callSync**: Validates enabled state (backend only), asserts no undefined args (debug only), then calls `callback.callSync()` with `obj` as the `this` object.

### DelayedItem

Created by `addDelayedListener()`. If `delayInMilliSeconds == 0`, falls back to regular `addListener()`.

**Constructor** (`ScriptBroadcaster.cpp:902`):
- Stores the function `f` and delay `ms` but does NOT create a `WeakCallbackHolder` immediately
- The callback is deferred - a new `DelayedFunction` is created on each `callSync()` invocation

**callSync** (`ScriptBroadcaster.cpp:911`):
- Creates a new `DelayedFunction` timer that fires after `ms` milliseconds
- Uses `parent->lastValues` (not the passed args) to ensure the latest state is used
- The previous `delayedFunction` is replaced (stopped and destroyed)
- When the timer fires (`DelayedFunction::timerCallback` at line 3799), it calls the callback via `WeakCallbackHolder::call()` with the captured args

### ComponentPropertyItem

Created by `addComponentPropertyListener()`. Has two operational modes:

**Mode 1: Without callback** (`optionalCallback == nullptr`):
- Directly sets the named property on all target components to the broadcast value
- Skips the source component (if `v == component` from args[0]) to avoid feedback loops
- Calls `sc->setScriptObjectPropertyWithChangeMessage(prop, value)` for each property in the `properties` list

**Mode 2: With callback** (`optionalCallback != nullptr`):
- The callback receives `(targetIndex, component, propertyId, value)` - the target index is prepended as the first argument
- `targetIndex` is the index of the current target component in the `obj` array (`obj.indexOf(v)`)
- The callback must return a value - returning undefined/void triggers `Result::fail("You need to return a value")`
- The returned value is then set as the property value on the target component

The callback is constructed with `numArgs + 1` parameters (broadcaster args + targetIndex).

### ComponentValueItem

Created by `addComponentValueListener()`. Has two modes:

**Mode 1: Without callback** (`optionalCallback == nullptr`):
- Sets the last broadcast argument (`args.getLast()`) as the value on all target components via `sc->setValue(v)`

**Mode 2: With callback** (`optionalCallback != nullptr`):
- Constructed at `ScriptBroadcaster.cpp:3097` with `numArgs + 1` parameters
- Callback receives `(targetIndex, ...broadcastArgs)` - targetIndex is `obj.indexOf(v)`
- Must return a value (undefined/void is an error)
- The returned value is set via `sc->setValue(rv)` on the target component

### ComponentRefreshItem

Created by `addComponentRefreshListener()`. **Has no callback** - the `refreshType` string parameter determines the action.

Constructor stores the refresh mode string and parses it to the `RefreshType` enum. Creates a `RefCountedTime` slot per target component for debug visualization timing.

`callSync()` ignores the incoming args entirely - it unconditionally triggers the specified refresh action on all target components.

### ModuleParameterSyncer

Created by `addModuleParameterSyncer()`. Forces synchronous execution (line 3586: `setForceSynchronousExecution(true)`).

**callSync** (`ScriptBroadcaster.cpp:920`):
```cpp
Result ScriptBroadcaster::ModuleParameterSyncer::callSync(const Array<var>& args)
{
    auto v = (float)args.getLast();
    FloatSanitizers::sanitizeFloatNumber(v);
    if(target.get() != nullptr)
        target->setAttribute(parameterIndex, v, sendNotificationAsync);
    return Result::ok();
}
```

Takes the last broadcast argument as a float value, sanitizes it, and sets it as the module parameter via `setAttribute()`. The `sendNotificationAsync` flag means the processor update happens asynchronously, but the syncer call itself is synchronous (forced by `setForceSynchronousExecution(true)`).

### OtherBroadcasterTarget

Created internally by `addBroadcasterAsListener()`, which is called from `attachToOtherBroadcaster()`. Forwards messages from one broadcaster to another.

**Constructor** (`ScriptBroadcaster.cpp:933`):
- Stores both `parent` (source) and `target` (destination) broadcasters as weak references
- Creates a `WeakCallbackHolder` for the `argTransformFunction` with `parent->defaultValues.size()` args

**callSync** (`ScriptBroadcaster.cpp:945`):
- If `argTransformFunction` is valid, calls it with the source broadcaster's args
- If the transform returns an array, that array is sent to the target broadcaster
- If the transform returns a non-array, the original args are forwarded unchanged
- Calls `target->sendMessageInternal(rv, async)` where `async` is the constructor parameter
- If no transform function: sends original args directly to target

---

## 15. Factory

### Engine.createBroadcaster(defaultValues)

At `ScriptingApi.cpp:2460`:

```cpp
juce::var ScriptingApi::Engine::createBroadcaster(var defaultValues)
{
    return var(new ScriptingObjects::ScriptBroadcaster(getScriptProcessor(), defaultValues));
}
```

Simple factory that constructs a `ScriptBroadcaster` and returns it as a `var`. The `defaultValues` parameter is passed directly to the `ScriptBroadcaster` constructor where it is parsed (see Section 4 for the four accepted formats). Registered as `API_METHOD_WRAPPER_1` at line 1292.

---

## 16. Threading

### Locks

| Lock | Type | Purpose | Location |
|---|---|---|---|
| `lastValueLock` | `SimpleReadWriteLock` | Protects `lastValues` array during read/write across threads | `ScriptBroadcaster.h:299` |
| `delayFunctionLock` | `CriticalSection` | Protects `currentDelayedFunction` replacement (for `callWithDelay`) | `ScriptBroadcaster.h:288` |

### Atomics

| Atomic | Type | Purpose |
|---|---|---|
| `asyncPending` | `std::atomic<bool>` | Coalesces rapid async sends - if true, new async sends are suppressed (unless queue mode is enabled). Reset to false when the async job completes. |

### Threading Model

**Synchronous sends** (`sendSyncMessage`, dot-assignment, forced sync):
- Execute on the calling thread
- `sendInternal()` iterates targets and calls `callSyncWithProfile()` directly
- In realtime-safe mode: no locks taken, args used directly
- In normal mode: read lock on `lastValueLock` per-target, args copied per-target

**Asynchronous sends** (`sendAsyncMessage`):
- Posts a `HiPriorityCallbackExecution` job to `JavascriptThreadPool` at line 3768:
  ```cpp
  pool.addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution,
      dynamic_cast<JavascriptProcessor*>(getScriptProcessor()), f);
  ```
- `HiPriorityCallbackExecution` ensures the job runs at elevated priority on the scripting thread
- With queue mode: each async message captures its own `lastValues` snapshot
- Without queue mode: the job reads `lastValues` at execution time (may have changed since posting)

**Backend safety check** (`ScriptBroadcaster.cpp:3662`):
```cpp
#if USE_BACKEND
    if(isSync && getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread() ==
       MainController::KillStateHandler::TargetThread::AudioThread)
    {
        if(!isRealtimeSafe())
            reportScriptError("You need to enable realtime safe execution...");
    }
#endif
```

This prevents synchronous execution from the audio thread without explicit realtime-safe mode, which would risk priority inversion or unsafe operations.

---

## 17. Preprocessor Guards

| Macro | Usage in Broadcaster | Effect |
|---|---|---|
| `PERFETTO` | `ScriptBroadcaster.h:40-54` | Enables Perfetto tracing macros (`OPEN_BROADCASTER_TRACK`, `TERMINATE_BROADCASTER_TRACK`, `CONTINUE_BROADCASTER_TRACK`) for flow-based performance visualization |
| `USE_BACKEND` | `ScriptBroadcaster.cpp:3661, 3839, 3433, 4587, etc.` | Guards backend-only features: audio thread safety check, debug breakpoint handling, `ScriptCallListener` debug visualization, `RealtimeSafetyInfo::check()`, `OtherBroadcasterListener::registerSpecialBodyItems()` |
| `HISE_NEW_PROCESSOR_DISPATCH` | `ScriptBroadcaster.cpp:1045, 1054, 1100` | Switches `ModuleParameterListener::ProcessorListener` base class between `Processor::AttributeListener` (new dispatch) and `Processor::OtherListener` (legacy dispatch). Affects how parameter change notifications are received |
| `HISE_INCLUDE_PROFILING_TOOLKIT` | `ScriptBroadcaster.cpp:3643` | Guards `ProfileCollection` instrumentation in `sendMessageInternal()` - captures data items and timing for profiling sessions. Adds `DebugSession::DataItem` with broadcaster args |
| `JUCE_DEBUG` | `ScriptBroadcaster.cpp:888` | Debug assertion in `ScriptTarget::callSync()` that args contain no undefined/void values |

---

## 18. BroadcasterMap

### ScriptBroadcasterMap

Defined in `ScriptBroadcasterMap.h:293`. Used as a floating tile panel with type ID `"ScriptBroadcasterMap"` (registered in `ScriptingPanelTypes.h:635`).

```cpp
class ScriptBroadcasterMap : public Component,
                             public ComponentWithPreferredSize,
                             public ControlledObject,
                             public ProcessorHelpers::ObjectWithProcessor,
                             public GlobalScriptCompileListener,
                             public AsyncUpdater
```

**Purpose**: Provides a visual debug map of all broadcaster connections in the HISE IDE. Shows:
- All broadcasters with their metadata (id, colour)
- Source attachments (ListenerBase items)
- Target listeners (TargetBase items)
- Message flow visualization with blink indicators via `MessageWatcher`

**MessageWatcher** (line 302): A timer-based component that tracks `lastMessageTime` on each broadcaster. When a broadcaster fires, the time changes and the watcher triggers a visual blink effect. Uses `LastTime` structs that compare `prevTime` against `bc->lastMessageTime`.

The map is declared as a `friend class` of `ScriptBroadcaster` (line 265 in the header), giving it direct access to `items`, `attachedListeners`, `lastValues`, and other internal state.

### Display (inline debug popup)

At `ScriptBroadcaster.cpp:296`, the `Display` struct is a popup component shown when clicking a broadcaster in the debug view. It provides:
- A text editor for manually sending values
- Reset button (calls `reset()`)
- Breakpoint button (sets `triggerBreakpoint`)
- Row components for each target with enable/disable toggles and goto-source buttons

---

## 19. Upstream Providers

These are `LambdaBroadcaster` instances in the HISE engine that provide events to Broadcaster's `ListenerBase` implementations.

### ProcessingSpecSource

- **Provider**: `MainController::specBroadcaster` (`LambdaBroadcaster<double, int>` at `MainController.h:2205`)
- **Accessor**: `getSpecBroadcaster()` at `MainController.h:1743`
- **Callback**: `ProcessingSpecSource::prepareCalled(ProcessingSpecSource& p, double sampleRate, int blockSize)` - stores args and sends async message to parent broadcaster
- **Registration**: Constructor subscribes via `mc->getSpecBroadcaster().addListener(*this, prepareCalled)`
- **Destructor**: Removes listener from `specBroadcaster`
- **Initial calls**: 0 (no initial value provided)

### NonRealtimeSource

- **Provider**: `MainController::realtimeBroadcaster` (`LambdaBroadcaster<bool>` at `MainController.h:2209`)
- **Accessor**: `getNonRealtimeBroadcaster()` at `MainController.h:1744`
- **Callback**: `NonRealtimeSource::onNonRealtimeChange(NonRealtimeSource& n, bool isNonRealtime)` - sends sync message
- **Note**: Sends **synchronously** (`sendSyncMessage`), unlike `ProcessingSpecSource` which sends async. This is because the realtime mode flag is used for critical audio path decisions.
- **Auto-enables realtime mode**: `attachToNonRealtimeChange()` sets `realtimeSafe = true` on the broadcaster
- **Initial calls**: 0

### InterfaceSizeListener

- **Provider**: `ScriptingContent::interfaceSizeBroadcaster` (`LambdaBroadcaster<int, int>` at `ScriptingApiContent.h:2931`)
- **Callback**: `InterfaceSizeListener::onUpdate(InterfaceSizeListener& il, int w, int h)` - sends async message
- **Registration**: Constructor subscribes directly: `sc->interfaceSizeBroadcaster.addListener(*this, onUpdate)`
- **Initial calls**: 1 - returns `[contentWidth, contentHeight]`

### ModuleParameterListener

- **Provider**: `Processor::AttributeListener` (new dispatch) or `Processor::OtherListener` (legacy dispatch) at `Processor.h:490`
- **Registration**: `addToProcessor(p, data, num, dispatch::DispatchType::sendNotificationAsyncHiPriority)` for regular parameters
- **Special providers**:
  - Bypass: `p->addBypassListener(this, dispatch::sendNotificationAsyncHiPriority)` via `Processor::BypassListener`
  - Intensity: `mod->intensityBroadcaster.addListener(*this, intensityChanged, true)` via `Modulation::intensityBroadcaster`

---

## 20. Send Variants

| Method | Sync/Async | Behavior |
|---|---|---|
| `sendMessage(args, isSync)` | Configurable | **Deprecated** - emits `debugError`. Delegates to `sendMessageInternal(args, isSync)` |
| `sendSyncMessage(args)` | Synchronous | Calls `sendMessageInternal(args, true)` |
| `sendAsyncMessage(args)` | Asynchronous | Calls `sendMessageInternal(args, false)` |
| `sendMessageWithDelay(args, ms)` | Deferred->Async | If `forceSync`, calls `sendMessage(args, true)`. Otherwise stores `pendingData = args`, starts `Timer` with `ms`. When timer fires (`timerCallback`), calls `sendMessage(pendingData, false)` (async). Only one pending delayed message at a time |
| `resendLastMessage(sync)` | Configurable | Sets `forceSend = true` (scoped), calls `sendMessageInternal(var(lastValues), isSync)`. The `forceSend` flag bypasses the change-detection gate so the message is always dispatched even if values haven't changed. If `forceSync` is active, overrides to sync |
| `callWithDelay(ms, argArray, function)` | Standalone | **Does NOT go through the listener system.** Creates a standalone `DelayedFunction` timer that calls the given function after `ms` milliseconds with `argArray`. Exclusive - replaces any pending delayed function (stopped and swapped under `delayFunctionLock`). Protected by `ScopedLock sl(delayFunctionLock)` |

### sendMessageInternal flow summary

1. `forceSync` override check
2. Backend audio-thread safety check (sync + audio thread + not realtime-safe = error)
3. Argument count validation
4. Realtime-safe fast path: skip locks, update lastValues, call sendInternal directly
5. Change detection against lastValues
6. If changed (or queue/forceSend): acquire write lock, update lastValues
7. Bypass check (store but don't dispatch)
8. Sync: call `sendInternal(lastValues)` directly
9. Async: post `HiPriorityCallbackExecution` to `JavascriptThreadPool` (with asyncPending gate unless queue mode)

---

## 21. Additional Patterns

### setForceSynchronousExecution

Sets `forceSync = true`. This forces all sends (including async requests) to execute synchronously. Auto-enabled by `addModuleParameterSyncer()` at line 3586 to ensure parameter synchronization happens immediately.

### setReplaceThisReference

Default: `true`. Controls whether the `obj` parameter passed to `addListener()` replaces the `this` reference in the callback function. When true, `var::NativeFunctionArgs(obj, ...)` is used as the callback invocation, making `this` inside the callback refer to the listener's object.

Set via `setReplaceThisReference(false)` if you want `this` to remain the broadcaster or the original script scope.

### Component Resolution

Used by `BroadcasterHelpers::getComponentsFromVar()` at line 174. Components can be specified as:

1. **String** - resolved via `content->getComponentWithName(Identifier(v.toString()))` (name lookup)
2. **Object** - resolved via `dynamic_cast<ScriptComponent*>(v.getObject())` (direct reference)
3. **Array** - each element is resolved individually using the above two methods

Null results from failed resolution are silently removed from the list (line 203-206).

### errorBroadcaster

```cpp
LambdaBroadcaster<ItemBase*, String> errorBroadcaster;
```

At `ScriptBroadcaster.h:344`. An internal `LambdaBroadcaster` that fires when errors occur during item operations. Used by `sendErrorMessage()` at line 4687:

```cpp
void ScriptBroadcaster::sendErrorMessage(ItemBase* i, const String& message, bool throwError)
{
    if (throwError)
        reportScriptError(message);
    else
        debugError(dynamic_cast<Processor*>(getScriptProcessor()), message);

    if (i != nullptr)
        errorBroadcaster.sendMessage(sendNotificationAsync, i, message);
}
```

This allows the `ScriptBroadcasterMap` or other debug tools to monitor broadcaster errors without being in the call chain.

### allowRefCount

```cpp
bool ScriptBroadcaster::allowRefCount() const
{ return false; }
```

At `ScriptBroadcaster.cpp:4885`. Returns `false`, meaning the broadcaster object cannot be reference-counted by `WeakCallbackHolder`. This is significant because it means the broadcaster is not prevented from being garbage collected by callback references - the `WeakCallbackHolder` will use weak references instead. This is by design since broadcasters are managed by the `JavascriptProcessor`'s callable object registry.

### ProfileCollection

```cpp
ProfileCollection broadcasterProfile;
```

At `ScriptBroadcaster.h:256`. Initialized in the constructor at line 3319:
```cpp
broadcasterProfile.setPrefix(parentId + "." + metadata.id.toString());
broadcasterProfile.setSourceType(DebugSession::ProfileDataSource::SourceType::Broadcaster);
broadcasterProfile.setColour(metadata.c.withAlpha(0.7f));
broadcasterProfile.setHolder(dynamic_cast<JavascriptProcessor*>(p), true);
broadcasterProfile.add(".sendMessage()");
broadcasterProfile.add(".callListeners()");
```

Two profile points are registered:
- Index 0: `".sendMessage()"` - profiled in `sendMessageInternal()` (guarded by `HISE_INCLUDE_PROFILING_TOOLKIT`)
- Index 1: `".callListeners()"` - profiled in `sendInternal()`

Each `ItemBase` (source/target) also gets its own profile index via `checkMetadataAndCallWithInitValues()` at line 4738: `i->profileIndex = broadcasterProfile.add(i->metadata.id.toString())`.

---

## 22. Cross-References

| Field | Value | Source |
|---|---|---|
| `createdBy` | `Engine` (`Engine.createBroadcaster`) | `class_survey_data.json` |
| `seeAlso` | `GlobalCable`, `Timer`, `TransportHandler` | `class_survey_data.json` |
| `callbackDensity` | `0.84` | `class_survey_data.json` |
| `threadingExposure` | `0.5` | `class_survey_data.json` |
| `enrichmentGroup` | `event` | `class_survey.md` |

### Relationship to GlobalCable

Both Broadcaster and GlobalCable are message-passing systems. GlobalCable provides global named channels for simple value passing, while Broadcaster provides a structured event system with typed sources, multiple targets, metadata, priority sorting, and debug visualization.

### Relationship to Timer

The Broadcaster uses `private Timer` inheritance for `sendMessageWithDelay()`, and `callWithDelay()` uses a separate `DelayedFunction` timer. `DelayedItem` (from `addDelayedListener()`) also uses its own timer. Timer is a lower-level mechanism; Broadcaster builds event coordination on top of it.

### Relationship to TransportHandler

TransportHandler provides DAW transport state callbacks (play/stop/tempo). Broadcaster can observe processing spec changes (`attachToProcessingSpecs`) and realtime state changes (`attachToNonRealtimeChange`), which overlap with TransportHandler's domain. TransportHandler is typically used for tempo-synced behavior, while Broadcaster observes audio engine configuration changes.
