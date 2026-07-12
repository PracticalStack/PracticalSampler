# UserPresetHandler -- C++ Source Exploration

## Resources Consulted

- `enrichment/base/UserPresetHandler.json` -- 25 API methods
- `enrichment/resources/survey/class_survey.md` -- prerequisite row #9: UserPresetHandler is prerequisite of MacroHandler and MidiAutomationHandler
- `enrichment/resources/survey/class_survey_data.json` -- UserPresetHandler entry (createdBy: Engine, seeAlso: MacroHandler, MidiAutomationHandler)
- No prerequisite Readmes loaded (MacroHandler and MidiAutomationHandler not yet processed; they depend on this class, not the reverse)

---

## Class Declaration

### Script Wrapper: ScriptUserPresetHandler

**Header:** `HISE/hi_scripting/scripting/api/ScriptExpansion.h` (line 42)

```cpp
class ScriptUserPresetHandler : public ConstScriptingObject,
                                public ControlledObject,
                                public MainController::UserPresetHandler::Listener
```

- Exposes as `"UserPresetHandler"` via `getObjectName()` returning `RETURN_STATIC_IDENTIFIER("UserPresetHandler")`
- Created by `ScriptingApi::Engine::createUserPresetHandler()` which returns `new ScriptUserPresetHandler(getScriptProcessor())`
- Constructor takes `ProcessorWithScriptingContent* pwsc`
- `ConstScriptingObject(pwsc, 0)` -- zero constants
- Registers as listener to `MainController::UserPresetHandler` in constructor, deregisters in destructor

### Core Infrastructure: MainController::UserPresetHandler

**Header:** `HISE/hi_core/hi_core/MainController.h` (line 610)

```cpp
class UserPresetHandler: public Dispatchable,
                         public AudioProcessorListener
```

This is a nested class within `MainController`. It is the C++ infrastructure that the scripting wrapper delegates to. Key roles:

1. **Preset load/save lifecycle** -- loading from files, ValueTree, applying presets to the module tree
2. **Custom automation** -- declaring named automation slots that map to host (DAW) parameters
3. **Host parameter integration** -- `AudioProcessorListener` for gesture callbacks
4. **Listener pattern** -- notifying registered `Listener` objects about preset changes
5. **State management** -- coordinating `UserPresetStateManager` instances for modular preset data

**Implementation files:**
- `HISE/hi_core/hi_core/UserPresetHandler.cpp` -- core UPH logic (990 lines)
- `HISE/hi_core/hi_core/MainController.cpp` -- additional UPH methods (lines ~2477-2686)
- `HISE/hi_scripting/scripting/api/ScriptExpansion.cpp` -- ScriptUserPresetHandler wrapper (lines 36-1119)

---

## Constructor Analysis (ScriptUserPresetHandler)

Source: `ScriptExpansion.cpp` line 66

```cpp
ScriptUserPresetHandler::ScriptUserPresetHandler(ProcessorWithScriptingContent* pwsc) :
    ConstScriptingObject(pwsc, 0),  // 0 constants
    ControlledObject(pwsc->getMainController_()),
    preCallback(pwsc, nullptr, var(), 1),
    postCallback(pwsc, nullptr, var(), 1),
    postSaveCallback(pwsc, nullptr, var(), 1),
    customLoadCallback(pwsc, nullptr, var(), 1),
    customSaveCallback(pwsc, nullptr, var(), 1),
    parameterGestureCallback(pwsc, nullptr, var(), 2)  // 2 args
```

**WeakCallbackHolder initialization:**
- `preCallback` -- 1 arg
- `postCallback` -- 1 arg
- `postSaveCallback` -- 1 arg
- `customLoadCallback` -- 1 arg
- `customSaveCallback` -- 1 arg
- `parameterGestureCallback` -- 2 args (but actually called with 3: type, slotIndex, startGesture -- see below)

Wait, looking more carefully at `onParameterGesture`: it passes 3 args:
```cpp
var args[3];
args[0] = type;          // (int) HisePluginParameterBase::Type
args[1] = slotIndex;     // int
args[2] = startGesture;  // bool
var::NativeFunctionArgs a(var(this), args, 3);
auto ok = parameterGestureCallback.callSync(a, nullptr);
```

But the WeakCallbackHolder is initialized with 2. This uses `callSync(NativeFunctionArgs)` rather than `callSync(args, numArgs, rv)`, which may have different behavior.

### No Constants

The constructor passes 0 to `ConstScriptingObject`, so no constants are registered.

### Method Registration

**Typed API methods (ADD_TYPED_API_METHOD_N):**

| Method | Param Types |
|--------|-------------|
| `setPostCallback` | `Function` |
| `setPostSaveCallback` | `Function` |
| `setPreCallback` | `Function` |
| `attachAutomationCallback` | `String`, `Function`, `Number` |
| `setParameterGestureCallback` | `Function` |

**Callback diagnostics registered:**

| Callback | Method | Priority |
|----------|--------|----------|
| `postCallback` | `setPostCallback` | 0 |
| `postSaveCallback` | `setPostSaveCallback` | 0 |
| `preCallback` | `setPreCallback` | 0 |
| `parameterGestureCallback` | `setParameterGestureCallback` | 0 |
| `attachAutomationCallback` | (raw diagnostic) | `checkCallbackNumArgs<2, 1>` |

The raw diagnostic for `attachAutomationCallback` uses `WeakCallbackHolder::checkCallbackNumArgs<2, 1>` meaning it expects the callback to take 2 arguments, with parameter index 1 (the second parameter, the function).

**All other methods use plain `ADD_API_METHOD_N`** -- no forced types.

---

## Listener Interface

**Header:** `MainController.h` line 842

```cpp
class Listener
{
public:
    virtual ~Listener() {};
    virtual void presetChanged(const File& newPreset) = 0;
    virtual void presetSaved(const File& newPreset) {};
    virtual void presetListUpdated() = 0;
    virtual ValueTree prePresetLoad(const ValueTree& dataToLoad, const File& fileToLoad) { return dataToLoad; };
    virtual void loadCustomUserPreset(const var& dataObject) {};
    virtual var saveCustomUserPreset(const String& presetName) { return {}; }
    virtual void onParameterGesture(bool startGesture, int parameterIndex) {}
};
```

ScriptUserPresetHandler implements all of these:
- `presetChanged` -> calls `postCallback` asynchronously
- `presetSaved` -> calls `postSaveCallback`
- `presetListUpdated` -> empty
- `prePresetLoad` -> calls `preCallback` synchronously, optionally converts ValueTree to/from JSON
- `loadCustomUserPreset` -> calls `customLoadCallback` synchronously with script lock
- `saveCustomUserPreset` -> calls `customSaveCallback` synchronously with script lock
- `onParameterGesture` -> calls `parameterGestureCallback` synchronously

---

## UserPresetStateManager System

**Header:** `PresetHandler.h` line 115

```cpp
class UserPresetStateManager: public RestorableObject
{
public:
    using Ptr = WeakReference<UserPresetStateManager>;
    using List = Array<Ptr>;
    
    virtual Identifier getUserPresetStateId() const = 0;
    virtual void resetUserPresetState() = 0;
    bool restoreUserPresetState(const ValueTree& root);
    void saveUserPresetState(ValueTree& presetRoot) const;
};
```

The UserPresetHandler maintains a list of state managers (`stateManagers`). During preset load/save, it iterates through them by ID. The predefined state IDs are:

```cpp
namespace UserPresetIds
{
    DECLARE_ID(MPEData);
    DECLARE_ID(MidiAutomation);
    DECLARE_ID(Modules);
    DECLARE_ID(Preset);
    DECLARE_ID(CustomJSON);
    DECLARE_ID(AdditionalStates);
}
```

**Known state managers:**
- `MidiControllerAutomationHandler` (implements `UserPresetStateManager`) -- ID: `MidiAutomation`
- `ModuleStateManager` (implements `UserPresetStateManager`) -- ID: `Modules`
- `MacroControlBroadcaster::MacroChainData` (via `UserPresetStateManager`) -- not a direct implementer but macro state is saved/loaded separately
- `CustomStateManager` (inner class of UserPresetHandler) -- ID: `CustomJSON`

### Preset Load Sequence (loadUserPresetInternal)

Source: `UserPresetHandler.cpp` line 587

The load sequence is:

1. Set `currentThreadThatIsLoadingPreset` to current thread handle (for `isCurrentlyInsidePresetLoad()`)
2. Record `timeOfLastPresetLoad`
3. Skip preloading during load
4. **Restore macro connections** (if macros enabled on frontend) -- loads macro routing first so control callbacks can reference correct macros
5. Choose data model:
   - If `USE_RAW_FRONTEND`: use raw data holder
   - If custom data model: restore `CustomJSON` state manager -> calls `loadCustomUserPreset` on listeners
   - Otherwise: find the front-end interface script processor and call `restoreAllControlsFromPreset`
6. Restore `Modules` state manager
7. Restore `MidiAutomation` state manager
8. Restore `MPEData` state manager
9. Restore macro **values** (after all connections are loaded)
10. Restore `AdditionalStates` (any non-special state managers)
11. Call `postPresetLoad()` which dispatches `presetChanged` to listeners on the message thread
12. Preload everything

**Key detail:** Steps 4 and 9 show that macros are loaded in two phases: connections first (step 4), then values (step 9). This ensures macro-controlled parameters have correct routing before values are applied.

**Key detail:** The `HISE_MACROS_ARE_PLUGIN_PARAMETERS` preprocessor affects whether macro state is restored from presets in all cases or only for internal/non-exclusive presets.

---

## CustomAutomationData -- Deep Infrastructure

### Class Declaration

**Header:** `MainController.h` line 653

```cpp
struct CustomAutomationData : public ReferenceCountedObject,
    public dispatch::SourceOwner,    // new dispatch system
    public ControlledObject
```

This is the core data class for each custom automation slot. Each instance represents one named automation parameter that can:
- Connect to module parameters (ProcessorConnection)
- Connect to other automation slots (MetaConnection)
- Connect to global cables (CableConnection)
- Be exposed as a DAW plugin parameter
- Be controlled via MIDI CC (via MidiControllerAutomationHandler)
- Be listened to by script callbacks (via AttachedCallback)

### JSON Schema for setCustomAutomation

The constructor (`CustomAutomationData::CustomAutomationData`) parses a JSON object with these properties:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ID` | String | (required) | Unique automation identifier |
| `min` | float | 0.0 | Range minimum |
| `max` | float | 1.0 | Range maximum |
| `middlePosition` | float | (none) | If set, applies skew so this value is at center |
| `stepSize` | float | 0.0 | Quantization step |
| `defaultValue` | float | range.start | Default value (clamped to range) |
| `allowMidiAutomation` | bool | true | Whether MIDI CC can control this slot |
| `allowHostAutomation` | bool | true | Whether DAW host can control this slot |
| `pluginParameterGroup` | String | "" | Plugin parameter group name (must be registered via setPluginParameterGroupNames) |
| `connections` | Array | (required) | Array of connection target objects |
| `mode` | String | (none) | Value-to-text converter mode (e.g., "Frequency", "Decibel") |
| `options` | Array | (none) | String array of discrete options (alternative to mode) |
| `suffix` | String | "" | Suffix for text display (when neither mode nor options is set) |

### Connection Target JSON Schema

Each connection object in the `connections` array is parsed by `CustomAutomationData::parse()`. Exactly one of these identification patterns must be present:

**ProcessorConnection:**
```json
{ "processorId": "MyEffect", "parameterId": "Frequency" }
```
Looks up the processor by name in the module tree and resolves the parameter by identifier.

**MetaConnection:**
```json
{ "automationId": "AnotherSlotId" }
```
Forward-references another automation slot within the same list. The referenced slot must appear earlier in the array.

**CableConnection:**
```json
{ "cableId": "MyCable" }
```
Connects to a global routing cable. Uses `GlobalRoutingManager::Helpers::getOrCreate()` to get or create the cable.

### Connection Types (Inner Classes)

1. **ProcessorConnection** -- directly sets a processor attribute via `setAttribute(paramIndex, value, dispatchType)`
2. **MetaConnection** -- calls another `CustomAutomationData::call()` recursively (meta-parameter pattern)
3. **CableConnection** -- bidirectional: sends to cable via `Cable::sendValue()`, receives via `CableTargetBase::sendValue()`. Uses a `recursive` flag to prevent feedback loops. The CableConnection does range conversion (0-1 normalization) when communicating with the cable.

### Value Flow

When `CustomAutomationData::call(float newValue, DispatchType n)` is invoked:
1. Sanitizes and clamps the value to range
2. Snaps to legal value (stepSize)
3. Stores `lastValue` and `args[0]=index, args[1]=lastValue`
4. If notification is not `dontSendNotification`: iterates `connectionList` and calls each connection
5. Dispatches value through the dispatch system (new or old)

---

## ValueToTextConverter Modes

Used by the `mode` property in custom automation data JSON. Available modes from `ValueToTextConverter::getAvailableTextConverterModes()`:

| Mode String | Display Format | Example |
|-------------|---------------|---------|
| `"Frequency"` | Hz/kHz | "440 Hz", "1.2 kHz" |
| `"Time"` | ms/s | "100ms", "1.5s" |
| `"TempoSync"` | tempo name | "1/4", "1/8T" |
| `"Pan"` | L/C/R | "50L", "C", "30R" |
| `"NormalizedPercentage"` | 0-100% (value 0.0-1.0) | "50%" |
| `"Decibel"` | dB | "-6.0 dB" |
| `"Semitones"` | st | "+2 st", "-12 st" |

Alternative: pass `options` array of strings for discrete choice display.
Fallback: uses `stepSize` and `suffix` for basic numeric display.

---

## HisePluginParameterBase::Type Enum

Used by `sendParameterGesture(automationType, ...)`:

```cpp
enum class Type
{
    Macro,            // 0
    CustomAutomation, // 1
    ScriptControl,    // 2
    NKSWrapper,       // 3
    numTypes          // 4
};
```

---

## Dispatch Type System

The `sendMessage` / `isSynchronous` parameters in several methods use a magic number encoding:

```cpp
// ApiHelpers::getDispatchType(syncValue, getDontForFalse)
// Mapping:
//   true          -> sendNotificationSync
//   false         -> sendNotificationAsync (or dontSendNotification if getDontForFalse)
//   SyncMagicNumber    -> sendNotificationSync
//   AsyncMagicNumber   -> sendNotificationAsync
//   AsyncHiPriorityMagicNumber -> sendNotificationAsyncHiPriority
```

The magic numbers are int constants defined in ApiHelpers. The scripting API presents these as `true`/`false` with magic number fallbacks.

For `updateAutomationValues`, `getDontForFalse=true`, meaning `false` maps to `dontSendNotification` (no message sent, just update the value internally).

For `attachAutomationCallback`, `getDontForFalse=false`, meaning `false` maps to `sendNotificationAsync`.

---

## Preprocessing System (setEnableUserPresetPreprocessing)

When `enablePreprocessing=true`, the `prePresetLoad` callback receives a JSON object (converted from the ValueTree via `convertToJson`) instead of a File object.

### convertToJson Structure

Source: `ScriptExpansion.cpp` line 943

The returned JSON object has this structure:

```json
{
  "version": "1.0.0",
  "Content": [
    { "id": "Knob1", "value": 0.5, "type": "ScriptSlider", ... },
    { "id": "Button1", "value": 1, ... }
  ],
  "Modules": { ... },
  "MidiAutomation": { ... },
  "MPEData": { ... }
}
```

When `shouldUnpackComplexData=true`:
- Values starting with "JSON" are parsed as JSON objects
- Base64-encoded `data` properties are decoded back to their original form

After the pre-callback modifies this JSON, `applyJSON` converts it back to a ValueTree for the actual load.

### Data Flow Without Preprocessing

When `enablePreprocessing=false` (default), the pre-callback receives a `ScriptFile` object pointing to the preset file. The ValueTree passes through unmodified.

---

## Custom Data Model (setUseCustomUserPresetModel)

Enabling the custom data model:
1. Creates a `CustomStateManager` instance (inner class)
2. The `CustomStateManager` registers itself as a `UserPresetStateManager` with ID `CustomJSON`
3. During preset load: `CustomStateManager::restoreFromValueTree` converts the ValueTree child to a var and calls `loadCustomUserPreset` on all listeners (which triggers `customLoadCallback`)
4. During preset save: `CustomStateManager::exportAsValueTree` calls `saveCustomUserPreset` on listeners (which triggers `customSaveCallback`)

The `usePersistentObject` flag is stored but its behavioral impact is within the broader preset system -- it controls whether the custom data persists between preset saves.

**Requirement:** `setCustomAutomation` requires the custom data model to be enabled first. The code:
```cpp
if (!getMainController()->getUserPresetHandler().setCustomAutomationData(newList))
{
    reportScriptError("you need to enable setUseCustomDataModel() before calling this method");
}
```

---

## Plugin Parameter Integration

### CustomAutomationParameter

Source: `PluginParameterProcessor.cpp` line 40

Each `CustomAutomationData` entry with `allowHost=true` generates a `CustomAutomationParameter` which:
- Extends `juce::AudioProcessorParameterWithID` (becomes a DAW plugin parameter)
- Extends `HisePluginParameterBase`
- Uses the automation's `id` as both parameter ID and name
- Uses the automation's `range` for normalization
- Uses the automation's `vtc` (ValueToTextConverter) for host display
- Reports `Type::CustomAutomation`
- Bidirectional: host changes -> `setValue` -> `data->call(sendNotificationSync)`, script changes -> dispatch listener -> `setValueNotifyingHost`

### Plugin Parameter Sort Function

`setPluginParameterSortFunction` installs a custom sorter on `PluginParameterAudioProcessor::pluginParameterSortFunction`. The sort callback receives two objects with:

| Property | Type | Description |
|----------|------|-------------|
| `type` | int | `HisePluginParameterBase::Type` enum value |
| `parameterIndex` | int | HISE parameter index |
| `typeIndex` | int | Slot index within type |
| `name` | String | Parameter name |
| `group` | String | Plugin parameter group name |

Return value: negative (first before second), zero (equal), positive (second before first). If the callback returns undefined/void, falls back to default sorting.

### Plugin Parameter Groups

`setPluginParameterGroupNames` stores a StringArray in `UserPresetHandler::pluginParameterGroups`. This is validated by `checkPluginParameterGroupName()` which returns a Result::fail if a group name used in automation data doesn't match the registered list.

---

## AttachedCallback System

Source: `ScriptExpansion.h` line 157, `ScriptExpansion.cpp` line 310

```cpp
struct AttachedCallback: public dispatch::ListenerOwner,
                         public ReferenceCountedObject
```

Each `attachAutomationCallback` call creates an AttachedCallback that:
1. Holds a reference to the target `CustomAutomationData`
2. Has two `WeakCallbackHolder` instances: `customUpdateCallback` (sync) and `customAsyncUpdateCallback` (async)
3. Registers with the new dispatch system via `cData->dispatcher.addValueListener`

The dispatch type determines which callback holder is used:
- `sendNotificationSync` -> `customUpdateCallback`, called synchronously on the audio thread
- `sendNotificationAsync` -> `customAsyncUpdateCallback`, called asynchronously

**Audio thread safety check:** In backend builds, synchronous callbacks are checked via `RealtimeSafetyInfo::check` to warn if the callback is not safe for audio thread execution.

**Removal:** Passing a non-function as `updateCallback` removes the existing callback for that automation ID. The code first removes any existing callback with the same ID, then only adds a new one if `updateCallback` is a JavaScript function.

The callback receives 2 arguments: `(automationIndex, newValue)`.

---

## UndoableUserPresetLoad

Source: `MainController.h` line 760

```cpp
struct UndoableUserPresetLoad : public ControlledObject,
                                public UndoableAction
```

When `setUseUndoForPresetLoading(true)` is enabled:
- `loadUserPresetFromValueTree` creates an `UndoableUserPresetLoad` and passes it to `mc->getControlUndoManager()`
- `perform()` loads the new preset
- `undo()` loads the old preset (captured at action creation time via `UserPresetHelpers::createUserPreset`)
- `createCoalescedAction` merges consecutive preset loads (keeps first old state, takes last new state)

This integrates with `Engine.undo()`.

---

## AutomationValueUndoAction

Source: `ScriptExpansion.cpp` line 472

```cpp
struct AutomationValueUndoAction: public UndoableAction
```

When `updateAutomationValues` is called with `useUndoManager=true`:
- Creates an `AutomationValueUndoAction` that captures old values before applying new ones
- `perform()` and `undo()` both call `updateAutomationValues` with `useUndoManager=false`
- This also integrates with `Engine.undo()`

---

## updateAutomationValues -- Dual Input Modes

Source: `ScriptExpansion.cpp` line 554

This method has two distinct input modes depending on `data` type:

**Mode 1: Integer input** -- `data` is an integer
```
Interprets as a "preferredProcessorIndex" and refreshes all automation values from
their processor connection states. No explicit values are set; it reads back current
processor attribute values.
```

**Mode 2: Array of objects** -- `data` is an Array
```json
[
  { "id": "MyParam", "value": 0.5 },
  { "id": "OtherParam", "value": 1.0 }
]
```
The array is sorted by automation index before applying values (to ensure consistent ordering). Each entry sets the corresponding automation data value.

**Error case:** If `data` is a DynamicObject (single object, not array), it throws a script error.

---

## isOldVersion -- Version Comparison

Source: `ScriptExpansion.cpp` line 247

```cpp
bool ScriptUserPresetHandler::isOldVersion(const String& version)
{
#if USE_BACKEND
    auto thisVersion = dynamic_cast<GlobalSettingManager*>(...)->getSettingsObject().getSetting(HiseSettings::Project::Version);
#else
    auto thisVersion = FrontendHandler::getVersionString();
#endif
    
    SemanticVersionChecker svs(version, thisVersion);
    return svs.isUpdate();
}
```

Compares the given version string against the current project version. Uses `SemanticVersionChecker` which parses `major.minor.patch` format. Returns true if the given version is OLDER than the current version (i.e., the current version is an update relative to the given version).

Backend gets the version from project settings; frontend gets it from `FrontendHandler::getVersionString()`.

---

## isCurrentlyLoadingPreset -- Thread-Aware

Source: `MainController.h` line 941

```cpp
bool isCurrentlyInsidePresetLoad() const 
{ 
    return LockHelpers::getCurrentThreadHandleOrMessageManager() == currentThreadThatIsLoadingPreset; 
};
```

This returns true only when called from the SAME thread that is performing the preset load. This prevents false positives when checking from a different thread. The `currentThreadThatIsLoadingPreset` is set via `ScopedValueSetter` in `loadUserPresetInternal()`.

---

## isInternalPresetLoad

Source: `MainController.h` line 939

```cpp
bool isInternalPresetLoad() const { return isInternalPresetLoadFlag; }
```

The flag is set by `ScopedInternalPresetLoadSetter` (inner struct, line 615). This is used when the preset load originates from a DAW state restore (plugin state) or initial state load, as opposed to a user explicitly selecting a preset. The flag is meaningful only during pre/post callbacks.

---

## DefaultPresetManager

Source: `MainController.h` line 910

```cpp
struct DefaultPresetManager: public ControlledObject
```

Manages the "default user preset" -- a preset that can be loaded as the initial state. Created by `initDefaultPresetManager()`.

- `resetToDefault()`: loads the default preset via `loadUserPresetFromValueTree` with `useUndoManagerIfEnabled=false`
- `getDefaultValue(componentId/componentIndex)`: retrieves individual default values for components

The default preset file is specified in project settings (`getCurrentFileHandler().getDefaultUserPreset()`). In backend, it's resolved relative to the UserPresets subdirectory. In frontend, the ValueTree is passed directly.

`resetToDefaultUserPreset()` in the script API calls `defaultPresetManager->resetToDefault()`, throwing a script error if no default preset manager exists.

---

## TagDataBase

Source: `MainController.h` line 801

```cpp
struct TagDataBase
```

A shared resource for preset tagging. Not directly exposed through the ScriptUserPresetHandler API methods (tag management is done through `Engine.setUserPresetTagList()`), but it's part of the UserPresetHandler infrastructure.

---

## Preprocessor Guards

| Preprocessor | Default | Impact |
|-------------|---------|--------|
| `USE_BACKEND` | varies | Affects version string source, factory paths, debug checks |
| `USE_FRONTEND` | varies | Frontend preset loading path, embedded preset extraction |
| `READ_ONLY_FACTORY_PRESETS` | 0 | Enables factory preset read-only protection via FactoryPaths |
| `HISE_MACROS_ARE_PLUGIN_PARAMETERS` | 0 | When 1, macros become plugin parameters; affects macro restoration during preset load |
| `USE_RAW_FRONTEND` | varies | Alternative frontend path that bypasses ScriptingContent |
| `USE_NEW_AUTOMATION_DISPATCH` | 1 | Uses new dispatch system (current default) |
| `USE_OLD_AUTOMATION_DISPATCH` | 0 | Legacy dispatch system (deprecated) |

---

## Preset Load Threading Model

1. `loadUserPreset(File)` -- parses XML, creates ValueTree, delegates to `loadUserPresetFromValueTree`
2. `loadUserPresetFromValueTree` -- if undo enabled, wraps in UndoableAction; otherwise:
   - Calls `preprocess(pendingPreset)` which iterates listeners for `prePresetLoad`
   - Sends all-notes-off
   - Calls `mc->killAndCallOnLoadingThread(f)` -- kills voices and schedules load on background thread
3. `loadUserPresetInternal` -- runs on loading thread:
   - Sets `currentThreadThatIsLoadingPreset`
   - Restores macro connections, then content/custom data, then MIDI automation, MPE, macro values, additional states
   - Calls `postPresetLoad()` which dispatches `presetChanged` to message thread via `callOnMessageThreadAfterSuspension`

**Key insight:** The pre-callback runs synchronously before the background thread is entered. The post-callback runs asynchronously on the message thread after the background load completes.

---

## createObjectForSaveInPresetComponents / updateSaveInPresetComponents

These two methods provide a round-trip for component values:

**createObjectForSaveInPresetComponents:**
1. Exports content as ValueTree via `content->exportAsValueTree()`
2. Strips `type` property from each child
3. Converts to dynamic object via `ValueTreeConverters::convertValueTreeToDynamicObject`

**updateSaveInPresetComponents:**
1. Converts dynamic object back to ValueTree
2. Re-adds `type` property by looking up each component by ID
3. Calls `content->restoreAllControlsFromPreset`

This is useful in the custom data model for manually managing which component values to save/restore.

---

## updateConnectedComponentsFromModuleState

Iterates all components in the scripting content and calls `updateValueFromProcessorConnection()` on each. This refreshes UI element values from their connected processor parameters -- useful after a module state has been changed programmatically.

---

## runTest -- Diagnostic Method

Source: `ScriptExpansion.cpp` line 817

Performs several diagnostic checks:
1. Reports stats: isCustomModel, numSaveInPreset, totalComponents, automationSlots, moduleStates
2. Checks connected components for:
   - Components connected to a processor but without `saveInPreset` enabled (warning)
   - Components connected to a processor that also has a module state (potential conflict)
3. If custom data model: tests save/load round-trip consistency (save -> load -> save -> compare)
4. Dumps module state information for each registered module state

Output goes to the console via `debugToConsole`.

---

## FactoryPaths (READ_ONLY_FACTORY_PRESETS)

Source: `UserPresetHandler.cpp` line 907

When `READ_ONLY_FACTORY_PRESETS=1`:
- `FactoryPaths` is a SharedResourcePointer
- Initialized from embedded frontend data (`FrontendFactory::getEmbeddedData`)
- Decompresses a ValueTree of factory preset paths
- `contains()` checks if a given file matches a factory preset path
- Used by `isReadOnly()` to prevent overwriting shipped presets

---

## Upstream Data Providers

### MainController
The `UserPresetHandler` is a nested class of `MainController` and holds a raw pointer `mc` to it. All module tree access, macro management, expansion handling, and lock-free dispatching flows through the MainController.

### MacroManager / MacroControlBroadcaster
During preset load, macro connections and values are restored via `mc->getMacroManager().getMacroChain()`. The macro system is a separate state component that the UPH coordinates but doesn't own.

### MidiControllerAutomationHandler
Custom automation data's `isConnectedToMidi()` queries the MIDI automation handler for active CC connections. The MIDI automation handler is a UserPresetStateManager that saves/restores its state as part of the preset.

### PluginParameterAudioProcessor
The host parameter system. Custom automation parameters are registered as JUCE AudioProcessorParameters. The sort function and group names are set on this class.

### GlobalRoutingManager
Cable connections in custom automation use the global routing cable system. The CableConnection subclass implements `CableTargetBase` for bidirectional value routing.

### Content (ScriptingContent)
Component value round-trips (`createObjectForSaveInPresetComponents`, `updateSaveInPresetComponents`, `updateConnectedComponentsFromModuleState`) operate on the scripting content tree.

---

## Key Design Patterns

1. **Two data models:** The default model stores component values in a ValueTree. The custom model delegates save/load to script callbacks. Custom automation requires the custom model.

2. **State manager composition:** The preset is not a monolithic blob -- it's composed of independently managed state segments (Content, Modules, MidiAutomation, MPEData, CustomJSON, AdditionalStates). Each has its own save/restore path.

3. **Connection abstraction:** Custom automation slots can connect to processors, other automation slots, or global cables through a polymorphic connection system. This enables both direct parameter control and meta-parameter routing.

4. **Dual callback timing:** Pre-callbacks are synchronous (can modify data before load). Post-callbacks are asynchronous (run on message thread after load completes). This separation enables both migration logic and UI updates.

5. **Undo integration:** Both preset loads and automation value changes can participate in the undo system, enabling `Engine.undo()` to restore previous states.
