# MidiAutomationHandler -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- MidiAutomationHandler row
- `enrichment/resources/survey/class_survey_data.json` -- MidiAutomationHandler entry
- `enrichment/phase1/MacroHandler/Readme.md` -- prerequisite (UserPresetHandler -> MacroHandler -> MidiAutomationHandler)
- `enrichment/phase1/UserPresetHandler/Readme.md` -- prerequisite (preset load lifecycle, custom automation integration)
- `enrichment/resources/explorations/MacroHandler.md` -- sibling class section covering ScriptedMidiAutomationHandler

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 2975

```cpp
class ScriptedMidiAutomationHandler : public ConstScriptingObject,
                                      public SafeChangeListener
{
public:
    struct Wrapper;

    ScriptedMidiAutomationHandler(ProcessorWithScriptingContent* sp);
    ~ScriptedMidiAutomationHandler();

    void changeListenerCallback(SafeChangeBroadcaster *b) override;

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("MidiAutomationHandler"); };
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MidiAutomationHandler"); }

    // API Methods (7)
    var getAutomationDataObject();
    void setAutomationDataFromObject(var automationData);
    void setControllerNumbersInPopup(var numberArray);
    void setControllerNumberNames(var ccName, var nameArray);
    void setExclusiveMode(bool shouldBeExclusive);
    void setUpdateCallback(var callback);
    void setConsumeAutomatedControllers(bool shouldBeConsumed);

private:
    MidiControllerAutomationHandler* handler;
    WeakCallbackHolder updateCallback;
};
```

### Inheritance

- **ConstScriptingObject** -- standard base for scripting objects with 0 constants (`ConstScriptingObject(sp, 0)`)
- **SafeChangeListener** -- receives async change notifications from `SafeChangeBroadcaster` (the handler)

The class is a thin scripting wrapper around the C++ `MidiControllerAutomationHandler` singleton.

### Scripting API Name

`getClassName()` returns `"MidiAutomationHandler"` -- this is the name exposed to HiseScript.

---

## Constructor

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 10074

```cpp
ScriptedMidiAutomationHandler::ScriptedMidiAutomationHandler(ProcessorWithScriptingContent* sp) :
    ConstScriptingObject(sp, 0),
    handler(sp->getMainController_()->getMacroManager().getMidiControlAutomationHandler()),
    updateCallback(getScriptProcessor(), this, var(), 1)
{
    handler->addChangeListener(this);

    ADD_API_METHOD_0(getAutomationDataObject);
    ADD_API_METHOD_1(setAutomationDataFromObject);
    ADD_API_METHOD_1(setControllerNumbersInPopup);
    ADD_API_METHOD_1(setExclusiveMode);
    ADD_API_METHOD_1(setUpdateCallback);
    ADD_API_METHOD_1(setConsumeAutomatedControllers);
    ADD_API_METHOD_2(setControllerNumberNames);
}
```

Key observations:

1. **0 constants** -- `ConstScriptingObject(sp, 0)` -- no script-visible constants
2. **Singleton access** -- `getMacroManager().getMidiControlAutomationHandler()` -- the handler is obtained from the MainController's MacroManager singleton. There is exactly one MidiControllerAutomationHandler per MainController.
3. **Change listener** -- subscribes to the handler's SafeChangeBroadcaster on construction, unsubscribes in destructor
4. **ALL `ADD_API_METHOD_N`** -- no `ADD_TYPED_API_METHOD_N` registrations. All parameter types must be inferred.
5. **WeakCallbackHolder** initialized with 1 argument slot

### Destructor

```cpp
ScriptedMidiAutomationHandler::~ScriptedMidiAutomationHandler()
{
    handler->removeChangeListener(this);
}
```

Clean removal from the change broadcaster.

---

## Wrapper Struct (Method Registration Types)

```cpp
struct ScriptedMidiAutomationHandler::Wrapper
{
    API_METHOD_WRAPPER_0(ScriptedMidiAutomationHandler, getAutomationDataObject);
    API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setAutomationDataFromObject);
    API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setControllerNumbersInPopup);
    API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setExclusiveMode);
    API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setUpdateCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setConsumeAutomatedControllers);
    API_VOID_METHOD_WRAPPER_2(ScriptedMidiAutomationHandler, setControllerNumberNames);
};
```

All use plain `ADD_API_METHOD_N` (no typed variants). No forced parameter types.

---

## Factory Method

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 2450

```cpp
juce::var ScriptingApi::Engine::createMidiAutomationHandler()
{
    return var(new ScriptingObjects::ScriptedMidiAutomationHandler(getScriptProcessor()));
}
```

Registered in the Engine class with `ADD_API_METHOD_0(createMidiAutomationHandler)` (line 1356). Each call creates a new wrapper, but all wrappers point to the same underlying `MidiControllerAutomationHandler` singleton.

---

## Underlying C++ Class: MidiControllerAutomationHandler

**File:** `HISE/hi_core/hi_core/MainControllerHelpers.h`, line 109

```cpp
class MidiControllerAutomationHandler : public UserPresetStateManager,
                                        public SafeChangeBroadcaster
```

### Key Properties

| Property | Description |
|----------|-------------|
| `exclusiveMode` | `bool`, default `false` -- when true, one CC can only control one parameter |
| `consumeEvents` | `bool`, default `true` -- when true, automated CC messages are removed from MIDI buffer |
| `filterChannels` | `bool` -- from `HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION` preprocessor |
| `controllerNumbersInPopup` | `BigInteger` -- bit mask of CC numbers to show in popup |
| `controllerNames` | `StringArray` -- custom display names for CC numbers |
| `ccName` | `String`, default `"MIDI CC"` -- category label for popup sections |
| `automationData` | `Container<AutomationData>` -- multi-dimensional storage of automation entries |
| `unlearnedData` | `AutomationData` -- pending MIDI learn entry |
| `anyUsed` | `bool` -- fast-path flag to skip processing when no automations exist |
| `mpeData` | `MPEData` -- nested MPE connection manager (separate system) |
| `unloadedData` | `ValueTree` -- deferred automation data for frontend startup |

### Interfaces

- **UserPresetStateManager** -- makes automation data a segment of user presets
  - `getUserPresetStateId()` returns `UserPresetIds::MidiAutomation` (identifier: "MidiAutomation")
  - `resetUserPresetState()` calls `clear(sendNotification)`
  - `exportAsValueTree()` / `restoreFromValueTree()` -- serialize/deserialize all automation entries
- **SafeChangeBroadcaster** -- notifies script wrappers and UI when automation data changes

### Access Path

```
MainController
  -> MacroManager (singleton)
    -> getMidiControlAutomationHandler()
      -> MidiControllerAutomationHandler (singleton)
```

---

## Container<AutomationData> Data Structure

The `Container<T>` template is a 3-dimensional array indexed by (channel, CC number):

```cpp
std::array<std::array<std::vector<T>, Key::MaxCCNumber>, Key::MaxChannel + 1> data;
```

Dimensions:
- 17 channel slots (0-15 for channels, slot 16 for omni/channel=-1)
- 128 CC number slots per channel
- Variable-length vector of `AutomationData` per (channel, CC) pair

### Key Struct

```cpp
struct Key {
    static constexpr int16 MaxCCNumber = 128;
    static constexpr int8 MaxChannel = 16;
    int8 channel = -2;   // -2 = invalid, -1 = omni, 0-15 = specific channel
    int16 ccNumber = 0;  // 0-127
};
```

The Key's `matchesOtherKey` method allows omni keys to match any channel. When `filterChannels` is false (default), all keys are created as omni (channel=-1).

### Iterator

`Container<T>::Iterator` provides flat iteration over all stored entries. Used for:
- Iterating all active automation entries
- Finding entries by processor/attribute
- Counting active connections
- Index-based random access into the flat list

The iterator has two modes:
- `justOmni=true` -- only iterates the omni channel data (channel=-1)
- `justOmni=false` -- iterates all 17 channel slots

The `createIterator()` method on `MidiControllerAutomationHandler` selects the mode based on `HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION`:

```cpp
MidiControllerAutomationHandler::Container<MidiControllerAutomationHandler::AutomationData>::Iterator
MidiControllerAutomationHandler::createIterator() const
{
    auto useChannels = HISE_GET_PREPROCESSOR(mc, HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION);
    return automationData.createIterator(!useChannels);
}
```

---

## AutomationData Struct

**File:** `HISE/hi_core/hi_core/MainControllerHelpers.h`, line 544

```cpp
struct AutomationData: public RestorableObject
{
    MainController* mc = nullptr;
    WeakReference<Processor> processor;
    int attribute;
    NormalisableRange<double> parameterRange;  // active sweep range
    NormalisableRange<double> fullRange;       // total parameter range
    float lastValue = -1.0f;
    int macroIndex;
    Key k;
    bool inverted = false;
    bool used;
    ValueToTextConverter textConverter;
};
```

### exportAsValueTree() Output Schema

Each automation entry serializes to a ValueTree child with tag `"Controller"`:

| Property | Type | Description |
|----------|------|-------------|
| `Controller` | int | CC number (0-127) |
| `Channel` | int | MIDI channel (1-based, or -1 for omni) |
| `Processor` | String | Processor ID |
| `MacroIndex` | int | Macro slot index (-1 if direct) |
| `Start` | double | Active range start |
| `End` | double | Active range end |
| `FullStart` | double | Full range start |
| `FullEnd` | double | Full range end |
| `Skew` | double | Range skew factor |
| `Interval` | double | Range step size |
| `Converter` | String | ValueToTextConverter serialization |
| `Attribute` | String | Parameter ID or custom automation ID |
| `Inverted` | bool | Whether mapping is inverted |

**Important:** The `Channel` property is stored as 1-based for consistency (the code explicitly states this in comments). When read back, channel value -1 means omni, and positive values are 1-based MIDI channels. The internal Key stores channels as 0-based (0-15) or -1 for omni.

### restoreFromValueTree() Attribute Resolution

The `Attribute` property resolution is complex:

1. If the Attribute value contains letters (is a string identifier, not a number):
   - First checks if custom automation data exists via `getUserPresetHandler().getNumCustomAutomationData()`
   - If custom automation exists, searches custom automation slots by ID string
   - Otherwise, resolves against the processor's parameter identifiers

2. If the Attribute is numeric:
   - Attempts legacy version migration via `UserPresetHelpers::getAutomationIndexFromOldVersion()`
   - Falls back to raw integer attribute index

### Range Handling

Two overlapping ranges are stored per connection (identical pattern to MacroHandler):

- **Active range** (`Start`/`End`/`Skew`/`Interval`): The sub-range that CC sweep maps to
- **Full range** (`FullStart`/`FullEnd`): The total parameter range

When restoring: `FullStart`/`FullEnd` default to `Start`/`End` if not present. This means if only active range is provided, full range equals active range.

### RangeHelpers IdSets

The range property names correspond to `RangeHelpers::IdSet::MidiAutomation` (Start, End, Interval, Skew) and `RangeHelpers::IdSet::MidiAutomationFull` (FullStart, FullEnd, Interval, Skew). These are defined in `HISE/hi_dsp_library/node_api/helpers/ParameterData.h`.

---

## JSON Schema (Script Layer)

The `getAutomationDataObject()` method converts the ValueTree to a JSON array via `ValueTreeConverters::convertFlatValueTreeToVarArray()`. Each entry becomes a JavaScript object with the same properties listed in the exportAsValueTree schema above.

The `setAutomationDataFromObject()` method converts back via `ValueTreeConverters::convertVarArrayToFlatValueTree(automationData, "MidiAutomation", "Controller")`.

These converter functions perform a flat property copy -- every ValueTree property becomes a JSON object property with the same name, and vice versa. No renaming, no nesting, no transformation.

### JSON Example (derived from source)

```javascript
[
    {
        "Controller": 1,      // CC number (Modulation Wheel)
        "Channel": -1,         // Omni
        "Processor": "Simple Gain1",
        "MacroIndex": -1,      // Direct mapping (not via macro)
        "Attribute": "Gain",   // Parameter ID
        "Start": 0.0,
        "End": 1.0,
        "FullStart": 0.0,
        "FullEnd": 1.0,
        "Skew": 1.0,
        "Interval": 0.01,
        "Inverted": false,
        "Converter": ""
    }
]
```

---

## handleControllerMessage() -- The Core Automation Dispatch

**File:** `HISE/hi_core/hi_core/MainControllerHelpers.cpp`, line 943

This is the runtime dispatch path that processes incoming CC messages:

1. Creates a `Key` from the HiseEvent (using omni if `filterChannels` is false)
2. Looks up all AutomationData entries for that Key
3. For each used entry with a valid processor:
   - Normalizes CC value to 0.0-1.0 (divides by 127)
   - Applies inversion if `inverted` flag is set
   - Converts from normalized to the `parameterRange`
   - Snaps to legal value
   - If `macroIndex != -1`: routes through MacroChain's `setMacroControl()`
   - Else if custom data model is active: calls `CustomAutomationData::call()`
   - Else: calls `processor->setAttribute()` directly
4. Returns `consumeEvents` flag if any automation matched (controls whether CC is removed from MIDI buffer)
5. Also checks omni assignments if the key was channel-specific

### Threading

`handleControllerMessage()` is called from `handleParameterData()` which processes a `MidiBuffer`. This runs on the **audio thread**. The `ScopedValueSetter` on line 959 temporarily disables plugin parameter change notifications during setAttribute -- MIDI CC changes should not propagate back as DAW parameter changes.

### consumeEvents Behavior

When `consumeEvents` is true (default), automated CC messages are removed from the MidiBuffer before it reaches script MIDI callbacks. When false, the CC message passes through to `onController` even after setting the automated parameter.

---

## Change Notification Flow

```
MidiControllerAutomationHandler (SafeChangeBroadcaster)
  -> sendChangeMessage() [async, message thread]
    -> ScriptedMidiAutomationHandler::changeListenerCallback()
      -> updateCallback.call1(getAutomationDataObject())
        -> Script callback receives current automation data as JSON array
```

`sendChangeMessage()` is called from:
- `restoreFromValueTree()` -- after loading automation data (sync if internal preset load, async otherwise)
- `setUnlearndedMidiControlNumber()` -- after MIDI learn completes
- `removeMidiControlledParameter()` -- after removing an automation entry
- `clear()` -- after clearing all automation data

The `changeListenerCallback` in the scripting wrapper calls `getAutomationDataObject()` to get a fresh snapshot and passes it to the user's callback. This means the callback always receives the complete current state, not a delta.

### Synchronous vs Asynchronous Notification

In `restoreFromValueTree()`:
```cpp
if(mc->getUserPresetHandler().isInternalPresetLoad())
    sendSynchronousChangeMessage();
else
    sendChangeMessage();
```

During preset loads, the notification is synchronous to ensure callbacks fire in the correct order within the preset load sequence. External changes (UI, MIDI learn) use async notification.

---

## setUpdateCallback Implementation

```cpp
void ScriptedMidiAutomationHandler::setUpdateCallback(var callback)
{
    if (HiseJavascriptEngine::isJavascriptFunction(callback))
    {
        updateCallback = WeakCallbackHolder(getScriptProcessor(), this, callback, 1);
        updateCallback.incRefCount();
        updateCallback.addAsSource(this, "onMidiAutomationUpdate");
        updateCallback.setThisObject(this);

        auto obj = getAutomationDataObject();
        auto r = updateCallback.callSync(&obj, 1);

        if (!r.wasOk())
            reportScriptError(r.getErrorMessage());
    }
}
```

Key behavior:
- **Immediate initial call** -- the callback is called synchronously once during registration with the current automation data
- **Error reporting** -- if the initial call fails, a script error is reported
- **Source tagging** -- tagged as `"onMidiAutomationUpdate"` for debugging
- **`this` binding** -- the MidiAutomationHandler instance is set as `this` context
- No mechanism to unregister the callback (setting a non-function value is a no-op; the old callback remains)

---

## Popup Configuration Methods

### setControllerNumbersInPopup

```cpp
void ScriptedMidiAutomationHandler::setControllerNumbersInPopup(var numberArray)
{
    BigInteger bi;
    if (auto a = numberArray.getArray())
    {
        for (auto v : *a)
            bi.setBit((int)v, true);
    }
    handler->setControllerPopupNumbers(bi);
}
```

Converts a JavaScript array of CC numbers to a `BigInteger` bitmask. Each number in the array sets a bit in the bitmask. The bitmask controls which CC numbers appear in the right-click automation popup on UI components.

**Behavioral impact in popup UI** (from `MacroControlledComponents.cpp`):
- When `hasSelectedControllerPopupNumbers()` is true (bitmask is non-zero):
  - The popup shows a flat list of allowed CCs directly (no "Learn" option, no submenu)
  - Section header reads "Assign {ccName}"
- When bitmask is zero (default):
  - The popup shows a "Learn" item at top level
  - CCs are in an "Assign {ccName}" submenu

The `shouldAddControllerToPopup()` method returns true for a CC number if:
- No custom popup numbers are set (bitmask is zero) -- all CCs shown
- The CC number's bit is set in the bitmask

The `isMappable()` method adds an additional exclusive-mode check: in exclusive mode, a CC number is only mappable if it has no existing automations across all channels.

### setControllerNumberNames

```cpp
void ScriptedMidiAutomationHandler::setControllerNumberNames(var ccName, var nameArray)
{
    handler->setCCName(ccName);
    
    StringArray sa;
    if (auto a = nameArray.getArray())
    {
        for (const auto& v : *a)
            sa.add(v.toString());
    }
    handler->setControllerPopupNames(sa);
}
```

Two effects:
1. `setCCName(ccName)` -- replaces the "MIDI CC" label used in popup section headers
2. `setControllerPopupNames(sa)` -- replaces the default "CC#N" names in the popup

The `getControllerName()` method returns:
- The custom name at index `controllerIndex` if available
- Otherwise falls back to `"CC#" + controllerIndex`

The `ccName` is used as the popup section header (e.g., "Assign {ccName}") and in the "Learn {ccName}" menu item.

---

## Exclusive Mode

```cpp
void MidiControllerAutomationHandler::setExclusiveMode(bool shouldBeExclusive)
{
    exclusiveMode = shouldBeExclusive;
}
```

Simple flag toggle. Effects:

1. **During MIDI learn** (`setUnlearndedMidiControlNumber`): When exclusive mode is active, assigning a CC to a parameter first removes all existing automations for that CC:
   - If `filterChannels` is active, removes all entries matching the key (across matching channels)
   - Otherwise, clears the entire vector for that CC (omni mode)
   Then adds the new assignment

2. **In popup** (`isMappable`): In exclusive mode, a CC number is only shown as mappable if no automation entries exist for it across all channels

Note: This is separate from the `MacroManager::setExclusiveMode()` which controls macro exclusivity. The MidiAutomationHandler has its own independent exclusive mode flag.

---

## Consume Automated Controllers

```cpp
void MidiControllerAutomationHandler::setConsumeAutomatedControllers(bool shouldConsume)
{
    consumeEvents = shouldConsume;
}
```

Default is `true`. When an incoming CC message matches an automation entry, the `handleControllerMessage` function returns the value of `consumeEvents`. The caller (`handleParameterData`) uses this return value to decide whether to include the message in the output buffer:

```cpp
consumed = handleControllerMessage(he);
// ...
if (!consumed) tempBuffer.addEvent(m, samplePos);
```

When `consumeEvents` is true, automated CC messages are eaten and never reach the script's `onController` callback. When false, they pass through.

---

## HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION Preprocessor

**File:** `HISE/hi_core/hi_core.h`, line 529

```
/** Config: HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION

If enabled, the plugin will use the MIDI channel information from CC messages 
when assigning a CC to a control. The default is disabled for backwards 
compatibility but you can enable this to assign the same CC number on different 
channels to different controls.

Note that this is a dynamic preprocessor so you don't need to recompile HISE 
to use this functionality, but just add HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION=1 
to your ExtraDefinitions.
*/
#ifndef HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION
#define HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION 0
#endif
```

This is a **dynamic preprocessor** -- it can be set via project ExtraDefinitions without recompiling HISE. When enabled:
- Keys include the MIDI channel (0-15), allowing the same CC number on different channels to control different parameters
- The iterator traverses all 17 channel slots (16 channels + omni)

When disabled (default):
- All keys are created as omni (channel=-1), ignoring MIDI channel
- The iterator only traverses the omni slot (128 entries)

This preprocessor is read at runtime via `HISE_GET_PREPROCESSOR()`:
- In the constructor: `filterChannels = HISE_GET_PREPROCESSOR(mc, HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION);`
- In `clear()`: re-reads the preprocessor value
- In `createIterator()`: determines iterator mode

---

## UserPresetStateManager Integration

`MidiControllerAutomationHandler` implements `UserPresetStateManager` with:
- `getUserPresetStateId()` returns `UserPresetIds::MidiAutomation` (the identifier `"MidiAutomation"`)
- `resetUserPresetState()` calls `clear(sendNotification)` -- removes all automation entries
- `exportAsValueTree()` / `restoreFromValueTree()` -- serialize/deserialize

This means MIDI automation data is automatically saved and restored as part of user presets. The preset load sequence (from UserPresetHandler Readme) shows MIDI automation restoration at step 6, after module states are restored.

### Unloaded Data Pattern

For frontend startup, there is a deferred loading mechanism:
```cpp
void setUnloadedData(const ValueTree& v);  // stores data for later
void loadUnloadedData();                   // restores stored data
```

`exportAsValueTree()` checks for unloaded data first and returns it if present:
```cpp
{
    SimpleReadWriteLock::ScopedReadLock sl(unloadLock);
    if (unloadedData.isValid())
        return unloadedData.createCopy();
}
```

This handles the case where automation data arrives before the processor tree is fully initialized (e.g., during frontend plugin startup when processors haven't been created yet).

---

## Macro Routing via MacroIndex

When an automation entry has `macroIndex != -1`, the CC message is routed through the macro system instead of directly setting the parameter:

```cpp
if (a.macroIndex != -1)
{
    a.processor->getMainController()->getMacroManager().getMacroChain()
        ->setMacroControl(a.macroIndex, (float)e.getControllerValue(), sendNotification);
}
```

The raw CC value (0-127) is passed directly to `setMacroControl()` -- the macro system handles its own normalization and range mapping. This bypasses the AutomationData's own `parameterRange`.

---

## Custom Automation Data Integration

When the UserPresetHandler is using a custom data model, CC automation targets custom automation slots instead of raw processor attributes:

```cpp
auto& uph = a.processor->getMainController()->getUserPresetHandler();
if (uph.isUsingCustomDataModel())
{
    if (auto ad = uph.getCustomAutomationData(a.attribute))
        ad->call(snappedValue, dispatch::DispatchType::sendNotificationSync);
}
else
{
    a.processor->setAttribute(a.attribute, snappedValue, sendNotificationSync);
}
```

The `a.attribute` field is reinterpreted as a custom automation slot index when the custom data model is active. The `ad->call()` method triggers the full custom automation chain (ProcessorConnections, MetaConnections, CableConnections) defined in the UserPresetHandler's `setCustomAutomation()` configuration.

---

## Parallel with MacroHandler

As documented in the MacroHandler exploration's sibling section, ScriptedMidiAutomationHandler follows the exact same pattern as ScriptedMacroHandler:

| Aspect | MacroHandler | MidiAutomationHandler |
|--------|-------------|----------------------|
| Underlying C++ class | MacroControlBroadcaster | MidiControllerAutomationHandler |
| Change pattern | MacroConnectionListener | SafeChangeListener |
| Get data | getMacroDataObject() | getAutomationDataObject() |
| Set data | setMacroDataFromObject() | setAutomationDataFromObject() |
| Update callback | setUpdateCallback() | setUpdateCallback() |
| Exclusive mode | setExclusiveMode() | setExclusiveMode() |
| **Extra methods** | (none) | setControllerNumbersInPopup, setControllerNumberNames, setConsumeAutomatedControllers |

The three extra methods in MidiAutomationHandler handle:
1. **Popup CC filtering** -- which CC numbers appear in the UI popup
2. **Popup CC naming** -- custom labels for CC numbers and category header
3. **Event consumption** -- whether automated CCs are eaten from the MIDI stream

---

## UI Consumer: MacroControlledObject Popup

**File:** `HISE/hi_core/hi_core/MacroControlledComponents.cpp`, around line 145

The popup menu for MIDI automation assignment is built in `MacroControlledObject::addAutomationMenuItems()`. It uses:
- `handler->shouldAddControllerToPopup(i)` -- filter CC numbers
- `handler->getControllerName(i)` -- display names
- `handler->isMappable(i)` -- grayed out in exclusive mode if already assigned
- `handler->getCCName()` -- section header text
- `handler->hasSelectedControllerPopupNumbers()` -- choose between flat list vs submenu layout

The popup also interacts with the custom data model: if `getNumCustomAutomationData() != 0`, MIDI assignment is only shown for controls with a valid `customId`.

---

## Threading Summary

| Operation | Thread | Notes |
|-----------|--------|-------|
| `handleControllerMessage()` | Audio thread | Reads automationData, calls setAttribute |
| `handleParameterData()` | Audio thread | Processes MidiBuffer, removes consumed events |
| `addMidiControlledParameter()` | Message thread | Takes ScopedLock on MainController lock |
| `removeMidiControlledParameter()` | Message thread | Takes AudioLock via LockHelpers |
| `restoreFromValueTree()` | Loading thread | Clears and rebuilds all entries, sends change message |
| `clear()` | Any | No explicit lock, relies on caller |
| `setControllerPopupNumbers()` | Message thread | Direct assignment (no lock) |
| `setControllerPopupNames()` | Message thread | Direct assignment (no lock) |
| `setExclusiveMode()` | Message thread | Simple bool flag |
| `setConsumeAutomatedControllers()` | Message thread | Simple bool flag |
| `changeListenerCallback()` | Message thread | Script callback dispatch |

The scripting API methods (set* methods) are all called from the script/message thread. The popup configuration methods (setControllerPopupNumbers, setControllerPopupNames, setCCName, setExclusiveMode, setConsumeAutomatedControllers) are simple state assignments with no locking -- they are designed to be set once during initialization.

---

## Preprocessor Summary

| Preprocessor | Type | Default | Effect |
|-------------|------|---------|--------|
| `HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION` | Dynamic | 0 | Enables per-channel CC assignment (same CC# on different channels can control different params) |

No other preprocessors guard the MidiAutomationHandler code. The `USE_BACKEND` preprocessor does not gate any of the 7 API methods.
