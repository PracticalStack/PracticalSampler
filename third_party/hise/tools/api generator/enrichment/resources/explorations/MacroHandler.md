# MacroHandler -- C++ Source Exploration

## Resources Consulted

- `enrichment/base/MacroHandler.json` -- authoritative method list (4 methods)
- `enrichment/resources/survey/class_survey_data.json` -- class metadata, seeAlso, createdBy
- `enrichment/resources/survey/class_survey.md` -- prerequisite table (MacroHandler has no prerequisites; it IS a prerequisite for UserPresetHandler)
- No base class exploration needed (ConstScriptingObject, not a component)

## Source File Locations

- **Header:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 2906-2973
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 9784-10059
- **Factory method:** `HISE/hi_scripting/scripting/api/ScriptingApi.h` line 649, `ScriptingApi.cpp` lines 3510-3513
- **Upstream: MacroControlBroadcaster:** `HISE/hi_core/hi_core/MacroControlBroadcaster.h` (full file, 360 lines)
- **Upstream: MacroManager:** `HISE/hi_core/hi_core/MainController.h` lines 391-458
- **Range helpers:** `HISE/hi_dsp_library/node_api/helpers/ParameterData.h` lines 72-133
- **Range store impl:** `HISE/hi_dsp_library/node_api/helpers/ParameterData.cpp` lines 225-266

---

## Class Declaration

```cpp
class ScriptedMacroHandler: public ConstScriptingObject,
                            public AsyncUpdater,
                            public MacroControlBroadcaster::MacroConnectionListener
```

### Inheritance

1. **ConstScriptingObject** -- standard scripting API base; provides `reportScriptError()`, `getScriptProcessor()`, etc.
2. **AsyncUpdater** (JUCE) -- allows coalescing listener notifications onto the message thread via `triggerAsyncUpdate()` / `handleAsyncUpdate()`.
3. **MacroControlBroadcaster::MacroConnectionListener** -- listener interface for macro connection changes. Pure virtual method: `macroConnectionChanged(int macroIndex, Processor* p, int parameterIndex, bool wasAdded)`.

### Scripting Class Name

Exposed as `"MacroHandler"` (via `RETURN_STATIC_IDENTIFIER("MacroHandler")`).

---

## Constructor

```cpp
ScriptingObjects::ScriptedMacroHandler::ScriptedMacroHandler(ProcessorWithScriptingContent* sp):
    ConstScriptingObject(sp, 0),                                  // 0 constants
    updateCallback(getScriptProcessor(), this, var(), 1)          // WeakCallbackHolder, 1 arg
{
    ADD_API_METHOD_0(getMacroDataObject);
    ADD_API_METHOD_1(setMacroDataFromObject);
    ADD_API_METHOD_1(setUpdateCallback);
    ADD_API_METHOD_1(setExclusiveMode);

    sp->getMainController_()->getMacroManager().getMacroChain()->addMacroConnectionListener(this);
}
```

**Key observations:**
- **Zero constants** -- `ConstScriptingObject(sp, 0)`. No `addConstant()` calls.
- **All methods use `ADD_API_METHOD_N`** (untyped), not `ADD_TYPED_API_METHOD_N`. No forced parameter types.
- Constructor registers itself as a `MacroConnectionListener` on the main macro chain.

### Destructor

```cpp
ScriptingObjects::ScriptedMacroHandler::~ScriptedMacroHandler()
{
    getScriptProcessor()->getMainController_()->getMacroManager().getMacroChain()->removeMacroConnectionListener(this);
}
```

Properly unregisters the listener.

---

## Method Wrapper Registration

```cpp
struct ScriptingObjects::ScriptedMacroHandler::Wrapper
{
    API_METHOD_WRAPPER_0(ScriptedMacroHandler, getMacroDataObject);
    API_VOID_METHOD_WRAPPER_1(ScriptedMacroHandler, setMacroDataFromObject);
    API_VOID_METHOD_WRAPPER_1(ScriptedMacroHandler, setUpdateCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptedMacroHandler, setExclusiveMode);
};
```

All use plain `API_METHOD_WRAPPER` / `API_VOID_METHOD_WRAPPER` -- no typed variants.

---

## Factory Method (obtainedVia)

In `ScriptingApi.h`:
```cpp
/** Creates a macro handler that lets you programmatically change the macro connections. */
var createMacroHandler();
```

In `ScriptingApi.cpp`:
```cpp
var ScriptingApi::Engine::createMacroHandler()
{
    return new ScriptingObjects::ScriptedMacroHandler(getScriptProcessor());
}
```

**Obtained via:** `Engine.createMacroHandler()`

---

## Internal Helper: ScopedUpdateDelayer

```cpp
struct ScopedUpdateDelayer
{
    ScopedUpdateDelayer(ScriptedMacroHandler& p, NotificationType n_):
      parent(p),
      prevValue(p.skipCallback),
      n(n_)
    {
        parent.skipCallback = true;
    }

    ~ScopedUpdateDelayer()
    {
        parent.skipCallback = prevValue;
        if(!parent.skipCallback)
            parent.sendUpdateMessage(n);
    };

    ScriptedMacroHandler& parent;
    bool prevValue = false;
    NotificationType n;
};
```

RAII guard that suppresses callback dispatch during bulk operations (used in `setMacroDataFromObject`). On destruction, it sends a single coalesced update if the callback was not already suppressed.

---

## Private Members

```cpp
bool skipCallback = false;
WeakCallbackHolder updateCallback;
```

- `skipCallback` -- gate to suppress callback dispatch during bulk macro rebuilds
- `updateCallback` -- JavaScript callback holder, initialized with 1 argument slot

---

## Private Methods

### sendUpdateMessage

```cpp
void ScriptingObjects::ScriptedMacroHandler::sendUpdateMessage(NotificationType n)
{
    if(updateCallback && n != dontSendNotification)
    {
        auto obj = getMacroDataObject();
        if(n == sendNotificationSync)
        {
            auto r = updateCallback.callSync(&obj, 1);
            if (!r.wasOk())
                reportScriptError(r.getErrorMessage());
        }
        else
        {
            updateCallback.call1(obj);
        }
    }
}
```

Dispatches the update callback with the full macro data object as its argument. Synchronous calls report errors; async calls are fire-and-forget.

### macroConnectionChanged (listener callback)

```cpp
void ScriptingObjects::ScriptedMacroHandler::macroConnectionChanged(int macroIndex, Processor* p,
    int parameterIndex, bool wasAdded)
{
    triggerAsyncUpdate();
}
```

Simply triggers an async update, which calls `handleAsyncUpdate()`:

```cpp
void ScriptingObjects::ScriptedMacroHandler::handleAsyncUpdate()
{
    sendUpdateMessage(sendNotificationAsync);
}
```

This means the JavaScript callback is always invoked asynchronously when a macro connection changes through the UI or other C++ code.

---

## JSON Schema: MacroIds Namespace

```cpp
namespace MacroIds
{
#define DECLARE_ID(x) static const Identifier x(#x);
    DECLARE_ID(MacroIndex);
    DECLARE_ID(Processor);
    DECLARE_ID(Attribute);
    DECLARE_ID(CustomAutomation);
#undef DECLARE_ID
}
```

These are the property keys used in the JSON objects returned by `getMacroDataObject()` and consumed by `setMacroDataFromObject()`.

---

## JSON Object Schema (getCallbackArg / getMacroDataObject)

The `getCallbackArg` method builds a `DynamicObject` with the following properties:

| Property | Type | Description |
|----------|------|-------------|
| `MacroIndex` | int | Macro slot index (0 to HISE_NUM_MACROS-1) |
| `Processor` | String | Processor ID of the connected module |
| `Attribute` | String | Parameter name on the processor (or custom automation ID) |
| `CustomAutomation` | bool | `true` if this is a custom automation connection (only set when true) |
| `FullStart` | double | Total range start (from MidiAutomationFull IdSet) |
| `FullEnd` | double | Total range end |
| `Start` | double | Parameter range start (from MidiAutomation IdSet) |
| `End` | double | Parameter range end |
| `Interval` | double | Step size |
| `Skew` | double | Skew factor |
| `Inverted` | bool | Whether the range is inverted |

The `getMacroDataObject` method returns an **Array** of these objects -- one per connected parameter across all macros.

### Range properties detail

Two sets of range properties are stored using `RangeHelpers::storeDoubleRange`:

1. **MidiAutomationFull** IdSet: `FullStart`, `FullEnd`, `Interval`, `Skew` -- represents the total range of the parameter
2. **MidiAutomation** IdSet: `Start`, `End`, `Interval`, `Skew`, `Inverted` -- represents the active sub-range within the total range

Both are stored on the same object, so the `Interval` and `Skew` properties from the second call overwrite the first (same key names).

---

## JSON Consumption Schema (setFromCallbackArg / setMacroDataFromObject)

`setMacroDataFromObject` expects an Array of objects with the same schema. Processing:

1. Validates required properties: `MacroIndex`, `Attribute`, `Processor`
2. Clears all existing macro connections (within a `ScopedUpdateDelayer`)
3. Iterates the array, calling `setFromCallbackArg` for each element

`setFromCallbackArg` details:

- `MacroIndex` -- int, must be in range `[0, HISE_NUM_MACROS)`
- `Processor` -- String, looked up via `ProcessorHelpers::getFirstProcessorWithName`
- `Attribute` -- String or int; if String, resolved via `getParameterIndexForIdentifier()` (or `getCustomAutomationData(Identifier)` if `CustomAutomation` is true)
- `CustomAutomation` -- bool, selects custom automation lookup path
- Range properties (`Start`/`End`/`FullStart`/`FullEnd`/`Interval`/`Skew`/`Inverted`) -- read via `RangeHelpers::getDoubleRange` with MidiAutomation and MidiAutomationFull IdSets
- `converter` -- String, deserialized via `ValueToTextConverter::fromString()`. This property is consumed but NOT produced by `getCallbackArg` -- it's an input-only property for display formatting.

### Range handling in setFromCallbackArg

```cpp
auto fr = RangeHelpers::getDoubleRange(obj, RangeHelpers::IdSet::MidiAutomationFull);
auto nr = RangeHelpers::getDoubleRange(obj, RangeHelpers::IdSet::MidiAutomation);

if (fr.getRange().isEmpty())
    fr = nr;  // Fall back to MidiAutomation range if Full range not specified

// ... addParameter with fr.rng (total range)

if(!RangeHelpers::isEqual(fr, nr) && !nr.rng.getRange().isEmpty())
{
    pd->setRangeStart(nr.rng.start);
    pd->setRangeEnd(nr.rng.end);
}

if (nr.inv)
    pd->setInverted(true);
```

This means:
- If only `Start`/`End` are provided (no `FullStart`/`FullEnd`), the active range is used as the total range.
- If both are provided and differ, the active range is set as a sub-range within the total range.
- Inversion is taken from the `MidiAutomation` range set.

---

## Upstream Infrastructure: MacroControlBroadcaster

`MacroControlBroadcaster` is a base class of `ModulatorSynthChain`. It manages all macro control slots.

### Key types

- **MacroControlData** -- one per macro slot; owns a list of `MacroControlledParameterData`
- **MacroControlledParameterData** -- represents a single parameter connection within a macro slot

### MacroControlData key methods

| Method | Description |
|--------|-------------|
| `addParameter(...)` | Adds a parameter connection with processor, index, name, converter, range |
| `removeParameter(int/String)` | Removes a parameter connection |
| `getNumParameters()` | Number of connected parameters |
| `getParameter(int)` | Access `MacroControlledParameterData` by index |
| `getParameterWithProcessorAndIndex(p, idx)` | Find by processor + parameter index |
| `setValue(float)` | Set macro value (0-127), propagates to all connected params |
| `getMacroName()` / `setMacroName(String)` | Display name |
| `setMidiController(int)` / `getMidiController()` | MIDI CC binding |

### MacroControlledParameterData key fields

| Field | Description |
|-------|-------------|
| `controlledProcessor` | WeakReference to target processor |
| `parameter` | Parameter index on the processor |
| `parameterName` | Display name |
| `range` | Total NormalisableRange (the full parameter range) |
| `parameterRange` | Active sub-range within the total range |
| `inverted` | Whether the mapping is inverted |
| `readOnly` | Whether the parameter can only be controlled by the macro |
| `customAutomation` | Whether this targets a CustomAutomationData slot |
| `textConverter` | ValueToTextConverter for display formatting |

### Listener pattern

```cpp
void sendMacroConnectionChangeMessage(int macroIndex, Processor* p, int parameterIndex, bool wasAdded, NotificationType n);
void sendMacroConnectionChangeMessageForAll(bool wasAdded);

void addMacroConnectionListener(MacroConnectionListener* l);
void removeMacroConnectionListener(MacroConnectionListener* l);
```

Listeners are stored as `WeakReference` in an `Array`, protected by a `CriticalSection`.

---

## Upstream Infrastructure: MacroManager

`MainController::MacroManager` is the top-level manager. Key features:

### Access chain

```
MainController -> MacroManager -> getMacroChain() -> MacroControlBroadcaster (on the ModulatorSynthChain)
```

### Exclusive mode

```cpp
void setExclusiveMode(bool shouldBeExclusive) { exclusiveMode = shouldBeExclusive; }
bool isExclusive() const { return exclusiveMode; }
```

When exclusive mode is enabled, a macro can only be connected to a single target -- connecting it to another target removes the old connection. This is the same `exclusiveMode` flag that `setExclusiveMode` on the ScriptedMacroHandler delegates to.

### MIDI-to-Macro binding

MacroManager also handles MIDI CC to macro mappings (separate from the MidiAutomationHandler system):

```cpp
void setMidiControllerForMacro(int midiControllerNumber);
void setMidiControllerForMacro(int macroIndex, int midiControllerNumber);
int getMacroControlForMidiController(int midiController);
int getMidiControllerForMacro(int macroIndex);
```

### Frontend macro control

```cpp
bool isMacroEnabledOnFrontend() const;
void setEnableMacroOnFrontend(bool shouldBeEnabled);
```

Controls whether macro controls are available in the exported plugin frontend.

---

## Preprocessor Configuration: HISE_NUM_MACROS

```cpp
// hi_core.h
#ifndef HISE_NUM_MACROS
#define HISE_NUM_MACROS 8
#endif

#ifndef HISE_NUM_MAX_MACROS
#define HISE_NUM_MAX_MACROS 64
#endif

#if HISE_NUM_MACROS > HISE_NUM_MAX_MACROS
#error "HISE_NUM_MACROS must not be bigger than HISE_NUM_MAX_MACROS"
#endif
```

- Default: **8 macros**
- Maximum: **64 macros**
- Configurable via project preprocessor definitions

### HISE_GET_PREPROCESSOR behavior

```cpp
// Macros.h
#if USE_BACKEND
#define HISE_GET_PREPROCESSOR(mc, x) mc->getExtraDefinitionsValue(#x, x)
#else
#define HISE_GET_PREPROCESSOR(this, x) x
#endif
```

In the HISE IDE (backend), the number of macros is read from the project's extra definitions at runtime. In exported plugins (frontend), the compile-time value is used directly. Both `getMacroDataObject` and `setMacroDataFromObject` use this macro to determine the number of active macro slots.

---

## CustomAutomation Integration

The macro handler has special logic for connecting macros to CustomAutomation parameters (from the UserPresetHandler):

- When `CustomAutomation` is `true` in the JSON object, the `Attribute` field is resolved via `UserPresetHandler::getCustomAutomationData()` instead of `Processor::getParameterIndexForIdentifier()`
- `getCallbackArg` checks `isCustomAutomation()` on each `MacroControlledParameterData` and overrides the `Attribute` with the custom automation ID string
- This allows macros to control custom host-automation parameters defined through the UserPresetHandler

---

## Threading Characteristics

- **Constructor/Destructor:** Must be called on the scripting thread (standard ConstScriptingObject lifecycle)
- **macroConnectionChanged:** Called from whichever thread triggers the macro connection change -- immediately delegates to `triggerAsyncUpdate()` (JUCE mechanism, coalesces to message thread)
- **handleAsyncUpdate:** Called on the message thread; calls `sendUpdateMessage` with async notification
- **setUpdateCallback:** Stores the callback, then immediately calls `sendUpdateMessage(sendNotificationSync)` to deliver the current state
- **setMacroDataFromObject:** Modifies macro connections synchronously on the calling thread (within ScopedUpdateDelayer)
- **getMacroDataObject:** Reads macro state synchronously -- no locking evident, relies on being called from the script thread

---

## Sibling Class: ScriptedMidiAutomationHandler

The `ScriptedMidiAutomationHandler` follows the exact same pattern as `ScriptedMacroHandler`:
- Same get/set data object pattern
- Same update callback pattern  
- Same exclusive mode delegation
- But targets `MidiControllerAutomationHandler` instead of `MacroControlBroadcaster`
- Uses `SafeChangeListener` instead of `MacroConnectionListener`
- Has additional methods: `setControllerNumbersInPopup`, `setControllerNumberNames`, `setConsumeAutomatedControllers`

This parallel design means they share a common architectural role in the preset-model domain.

---

## FrontendMacroPanel (UI consumer)

`FrontendMacroPanel` in `FrontendPanelTypes.h` is a `FloatingTile` component that also implements `MacroConnectionListener`. It provides a table-based UI for viewing and editing macro connections. This is the built-in UI counterpart to the scripting API -- the `MacroHandler` allows scripts to do programmatically what `FrontendMacroPanel` does visually.

---

## Summary

MacroHandler is a lightweight service wrapper (4 methods, 0 constants) around HISE's macro control infrastructure. It provides:

1. **Read** (`getMacroDataObject`) -- serialize all macro connections as a JSON array
2. **Write** (`setMacroDataFromObject`) -- rebuild all macro connections from a JSON array
3. **Watch** (`setUpdateCallback`) -- register a callback for macro connection changes
4. **Configure** (`setExclusiveMode`) -- toggle exclusive macro-to-target binding

The JSON schema uses `MacroIndex`, `Processor`, `Attribute`, `CustomAutomation` as core identifiers, plus range properties from the `MidiAutomation` and `MidiAutomationFull` RangeHelpers IdSets. The class bridges the C++ `MacroControlBroadcaster` / `MacroManager` infrastructure to HiseScript for programmatic macro management.
