# UserPresetHandler -- Method Documentation

## attachAutomationCallback

**Signature:** `undefined attachAutomationCallback(String automationId, Function updateCallback, Number isSynchronous)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.attachAutomationCallback("Volume", onVolumeChanged, SyncNotification);`

**Description:**
Attaches a script callback to a custom automation slot. The callback fires whenever the automation value changes, either from DAW host automation, MIDI CC, script calls to `setAutomationValue`, or `updateAutomationValues`. The `isSynchronous` parameter selects the dispatch mode: synchronous callbacks run on the audio thread (must use `inline function`), while asynchronous callbacks run on the UI thread. Passing a non-function value as `updateCallback` removes any previously attached callback for that automation ID. Only one callback can be attached per automation ID on a given UserPresetHandler instance -- attaching a new callback for the same ID replaces the previous one.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| automationId | String | yes | The ID of the custom automation slot to listen to. Must match an ID registered via `setCustomAutomation`. | Must match a registered automation ID |
| updateCallback | Function | yes | The callback function to invoke when the automation value changes. Pass a non-function (e.g., `false`) to remove the callback. | Must accept 2 arguments |
| isSynchronous | Number | yes | Dispatch mode for the callback. Use `SyncNotification` for audio-thread sync, `AsyncNotification` for UI-thread async. | `SyncNotification`, `AsyncNotification`, or `AsyncHiPriorityNotification` |

**Callback Signature:** updateCallback(automationIndex: int, newValue: double)

**Pitfalls:**
- Synchronous callbacks are checked for audio-thread safety in backend builds. If the callback is not an `inline function` or performs unsafe operations, HISE reports a script error at registration time (backend only). In exported plugins this check is absent, so the callback will still register but may cause audio glitches.

**Cross References:**
- `$API.UserPresetHandler.clearAttachedCallbacks$`
- `$API.UserPresetHandler.setCustomAutomation$`
- `$API.UserPresetHandler.setAutomationValue$`
- `$API.UserPresetHandler.updateAutomationValues$`

**Example:**
```javascript:automation-callback-sync
// Title: Listening to automation value changes synchronously
const var uph = Engine.createUserPresetHandler();

inline function onVolumeChanged(index, value)
{
    // Realtime-safe work only: update a reg variable, set module attribute, etc.
    Console.print("Automation " + index + " = " + value);
};

uph.attachAutomationCallback("Volume", onVolumeChanged, SyncNotification);
```
```json:testMetadata:automation-callback-sync
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with a full automation data array to register the 'Volume' ID first, and a DAW/host context to trigger automation value changes."
}
```

## clearAttachedCallbacks

**Signature:** `undefined clearAttachedCallbacks()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.clearAttachedCallbacks();`

**Description:**
Removes all automation callbacks that were previously attached via `attachAutomationCallback`. This clears the internal list of `AttachedCallback` objects, which deregisters them from the dispatch system and releases the callback holders. This method is also called automatically when the `UserPresetHandler` is destroyed.

**Parameters:**

(none)

**Cross References:**
- `$API.UserPresetHandler.attachAutomationCallback$`
- `$API.UserPresetHandler.setCustomAutomation$`

## createObjectForAutomationValues

**Signature:** `Array createObjectForAutomationValues()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Constructs new DynamicObject and Array on the heap for each automation slot.
**Minimal Example:** `var values = {obj}.createObjectForAutomationValues();`

**Description:**
Returns an array of objects representing the current values of all custom automation slots. Each element is an object with `id` (the automation slot's string identifier) and `value` (the current float value). This is the inverse of `updateAutomationValues` when used with the array-of-objects input mode -- the output of this method can be passed back to `updateAutomationValues` to restore automation state.

**Parameters:**

(none)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| id | String | The automation slot identifier matching the `ID` field from `setCustomAutomation` |
| value | Double | The current value of the automation slot |

**Cross References:**
- `$API.UserPresetHandler.updateAutomationValues$`
- `$API.UserPresetHandler.setCustomAutomation$`

**Example:**
```javascript:save-automation-values
// Title: Capturing current automation state for later restoration
const var uph = Engine.createUserPresetHandler();

// After custom automation has been set up:
var snapshot = uph.createObjectForAutomationValues();

// snapshot is an array like:
// [{"id": "Volume", "value": 0.75}, {"id": "Pan", "value": 0.5}]

// Restore later:
uph.updateAutomationValues(snapshot, SyncNotification, false);
```
```json:testMetadata:save-automation-values
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with full automation data setup prior to calling."
}
```

## createObjectForSaveInPresetComponents

**Signature:** `JSON createObjectForSaveInPresetComponents()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Exports a ValueTree and converts it to a DynamicObject, involving heap allocations and string operations.
**Minimal Example:** `var componentData = {obj}.createObjectForSaveInPresetComponents();`

**Description:**
Exports the current values of all UI components that have the `saveInPreset` flag enabled. Returns a JSON object derived from the internal ValueTree representation of the scripting content, with the `type` property stripped from each component entry. This is the read half of a round-trip pair -- use `updateSaveInPresetComponents` to restore values from the returned object. Primarily useful within the custom data model (via `setUseCustomUserPresetModel`) to manually include component state in custom preset data.

**Parameters:**

(none)

**Cross References:**
- `$API.UserPresetHandler.updateSaveInPresetComponents$`
- `$API.UserPresetHandler.setUseCustomUserPresetModel$`

## getAutomationIndex

**Signature:** `Integer getAutomationIndex(String automationID)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var idx = {obj}.getAutomationIndex("Volume");`

**Description:**
Returns the zero-based index of a custom automation slot by its string ID. Returns -1 if the custom data model is not active or if no automation slot with the given ID exists. The returned index can be used with `setAutomationValue` and corresponds to the position of the slot in the array passed to `setCustomAutomation`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| automationID | String | no | The ID string of the automation slot to look up. | Must match an ID registered via `setCustomAutomation` |

**Cross References:**
- `$API.UserPresetHandler.setAutomationValue$`
- `$API.UserPresetHandler.setCustomAutomation$`

## getSecondsSinceLastPresetLoad

**Signature:** `Double getSecondsSinceLastPresetLoad()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var elapsed = {obj}.getSecondsSinceLastPresetLoad();`

**Description:**
Returns the number of seconds elapsed since the last user preset was loaded. The timer starts when `loadUserPresetInternal` begins execution (at the start of the loading thread). Uses `Time::getMillisecondCounter()` internally, so the resolution is milliseconds but the return value is in seconds as a floating-point number. If no preset has been loaded since the plugin was instantiated, the counter starts from zero (the epoch of `getMillisecondCounter()`), which will produce a very large value representing the time since the application started.

**Parameters:**

(none)

**Cross References:**
- `$API.UserPresetHandler.isCurrentlyLoadingPreset$`
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setPostCallback$`

## isCurrentlyLoadingPreset

**Signature:** `Integer isCurrentlyLoadingPreset()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var loading = {obj}.isCurrentlyLoadingPreset();`

**Description:**
Returns true if this method is called from the same thread that is currently performing a preset load. The implementation compares the calling thread's handle against the thread handle stored at the start of `loadUserPresetInternal`. This thread-aware design prevents false positives when queried from a different thread while a preset load is in progress on another thread. Returns false if no preset load is in progress or if called from a different thread than the one performing the load. Useful inside automation callbacks, module state restoration, or other code that may execute during a preset load to distinguish preset-driven changes from user-driven changes.

**Parameters:**

(none)

**Cross References:**
- `$API.UserPresetHandler.isInternalPresetLoad$`
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setPostCallback$`

## isInternalPresetLoad

**Signature:** `Integer isInternalPresetLoad()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var isInternal = {obj}.isInternalPresetLoad();`

**Description:**
Returns true if the preset currently being loaded originates from a DAW state restore (plugin session recall) or initial state load, as opposed to the user explicitly selecting a preset from the preset browser or file system. The flag is set by a `ScopedInternalPresetLoadSetter` which saves and restores the previous value via RAII, so nested load operations preserve the outer context. This method is only meaningful when called during a pre-callback or post-callback -- outside those callbacks, the flag retains its last value from the most recent load, which may be stale.

**Parameters:**

(none)

**Pitfalls:**
- The flag is only reliably meaningful during pre/post callbacks. Calling this outside a preset load callback returns the stale value from the most recent load, which can produce incorrect logic if the code assumes the flag reflects the current state.

**Cross References:**
- `$API.UserPresetHandler.isCurrentlyLoadingPreset$`
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setPostCallback$`

## isOldVersion

**Signature:** `Integer isOldVersion(String version)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Accesses project settings in backend (GlobalSettingManager lookup) and FrontendHandler::getVersionString() in frontend; both involve String construction.
**Minimal Example:** `var isOld = {obj}.isOldVersion("1.0.0");`

**Description:**
Compares the given version string against the current project version and returns true if the given version is older. Uses semantic versioning (major.minor.patch format) via the internal `SemanticVersionChecker` class. In the HISE IDE (backend), the current version is read from the project settings (`HiseSettings::Project::Version`). In exported plugins (frontend), it is read from `FrontendHandler::getVersionString()`. This method is typically used inside a pre-callback to detect presets saved with an older plugin version and apply migration logic before loading.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| version | String | no | The version string to compare against the current project version. | Must be in "major.minor.patch" format (e.g., "1.2.0") |

**Cross References:**
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setEnableUserPresetPreprocessing$`

**Example:**
```javascript:version-migration-in-precallback
// Title: Applying migration logic for presets from older versions
const var uph = Engine.createUserPresetHandler();
uph.setEnableUserPresetPreprocessing(true, false);

uph.setPreCallback(function(presetData)
{
    local version = presetData.version;

    if (uph.isOldVersion(version))
    {
        // Migrate preset data from an older format
        Console.print("Migrating preset from version " + version);
    }
});
```
```json:testMetadata:version-migration-in-precallback
{
  "testable": false,
  "skipReason": "Requires a preset load trigger with a preset file containing a version string to invoke the pre-callback."
}
```

## resetToDefaultUserPreset

**Signature:** `undefined resetToDefaultUserPreset()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Triggers a full preset load cycle (loadUserPresetFromValueTree) which involves voice killing, thread dispatch, and ValueTree operations.
**Minimal Example:** `{obj}.resetToDefaultUserPreset();`

**Description:**
Loads the default user preset as defined in the project settings. The default preset is specified via the project's `DefaultUserPreset` setting and is managed by an internal `DefaultPresetManager`. Calling this triggers the full preset load lifecycle (pre-callback, voice kill, background thread load, post-callback). If no default preset has been configured in the project settings, a script error is thrown with the message "You need to set a default user preset in order to user this method". The load bypasses the undo manager even if `setUseUndoForPresetLoading` is enabled.

**Parameters:**

(none)

**Pitfalls:**
- Throws a script error if no default preset has been configured in the project settings. There is no query method to check whether a default preset exists before calling this.

**Cross References:**
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setPostCallback$`
- `$API.UserPresetHandler.setUseCustomUserPresetModel$`
- `$API.UserPresetHandler.setUseUndoForPresetLoading$`

## runTest

**Signature:** `undefined runTest()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Performs extensive String construction, ValueTree export, XML generation, and JSON serialization. Outputs to console via debugToConsole.
**Minimal Example:** `{obj}.runTest();`

**Description:**
Runs a diagnostic suite that checks for common user preset data persistency issues and prints a detailed report to the HISE console. The test performs these checks:

1. **Stats summary** -- reports whether the custom data model is active, the number of `saveInPreset` components, total components, automation slots, and module states.
2. **Connected component checks** -- for each component connected to a processor: warns if `saveInPreset` is not enabled (the connection value would not persist across presets), and warns if the connected processor also has a module state registered (potential conflict where both the component and the module state try to restore the same parameter).
3. **Custom data round-trip** (only if custom data model is active) -- performs a save-load-save cycle and compares the two saved outputs as JSON strings. Reports a warning if they differ, indicating data inconsistency in the custom save/load callbacks.
4. **Module state dump** -- for each registered module state, exports the module's ValueTree as XML and prints it to the console.

This is a development-time diagnostic tool. All output goes to the console via `debugToConsole`.

**Parameters:**

(none)

**Cross References:**
- `$API.UserPresetHandler.setUseCustomUserPresetModel$`
- `$API.UserPresetHandler.setCustomAutomation$`
- `$API.UserPresetHandler.createObjectForSaveInPresetComponents$`

## sendParameterGesture

**Signature:** `Integer sendParameterGesture(Integer automationType, Integer indexWithinType, Integer gestureActive)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Calls JUCE AudioProcessorParameter::beginChangeGesture/endChangeGesture which notify the DAW host; involves listener dispatch and potential string operations in the host layer.
**Minimal Example:** `{obj}.sendParameterGesture(1, 0, true);`

**Description:**
Sends a parameter gesture begin/end message to the DAW host for a specific plugin parameter. This notifies the host that the user is starting or ending an interaction with a parameter, which enables the host to record automation correctly (most DAWs only record automation changes between begin/end gesture pairs). The method iterates through all registered plugin parameters, finds the one matching the given type and index, and calls `beginChangeGesture()` or `endChangeGesture()` on it. Returns true if a matching parameter was found, false otherwise.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| automationType | Integer | no | The type of plugin parameter to target. | 0=Macro, 1=CustomAutomation, 2=ScriptControl, 3=NKSWrapper |
| indexWithinType | Integer | no | The zero-based index of the parameter within its type category. | Must be a valid index for the given type |
| gestureActive | Integer | no | Whether the gesture is starting (true) or ending (false). | Boolean value |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| 0 (Macro) | Targets a macro parameter slot |
| 1 (CustomAutomation) | Targets a custom automation slot registered via `setCustomAutomation` |
| 2 (ScriptControl) | Targets a script UI control parameter |
| 3 (NKSWrapper) | Targets an NKS integration parameter |

**Cross References:**
- `$API.UserPresetHandler.setParameterGestureCallback$`
- `$API.UserPresetHandler.setCustomAutomation$`
- `$API.UserPresetHandler.setAutomationValue$`
- `$API.UserPresetHandler.getAutomationIndex$`

## setAutomationValue

**Signature:** `Integer setAutomationValue(Integer automationIndex, Double newValue)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** The call path through CustomAutomationData::call is lock-free: sanitizes value, clamps to range, snaps to step, dispatches through connections. No allocations or locks in the core path.
**Minimal Example:** `{obj}.setAutomationValue(0, 0.75);`

**Description:**
Sets the value of a custom automation slot by its zero-based index. The value is dispatched synchronously (`sendNotificationSync`) through all connections attached to the automation slot (processor parameters, meta-connections, cable connections). Returns true if the custom data model is active and the index is valid, false otherwise. This is the primary method for programmatically driving automation values from script -- use it within `sendParameterGesture` begin/end pairs to ensure the DAW records the automation correctly. The value is sanitized, clamped to the slot's configured range, and snapped to the step size before being dispatched.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| automationIndex | Integer | no | The zero-based index of the custom automation slot. Use `getAutomationIndex` to convert from string ID. | Must be in range [0, numAutomationSlots) |
| newValue | Double | no | The new value for the automation slot. Clamped to the slot's configured range. | Clamped to [min, max] from setCustomAutomation |

**Pitfalls:**
- Returns false silently if the custom data model is not active or the index is out of range. No error is thrown, so the caller must check the return value to detect failure.

**Cross References:**
- `$API.UserPresetHandler.getAutomationIndex$`
- `$API.UserPresetHandler.setCustomAutomation$`
- `$API.UserPresetHandler.sendParameterGesture$`
- `$API.UserPresetHandler.attachAutomationCallback$`
- `$API.UserPresetHandler.updateAutomationValues$`

## setCustomAutomation

**Signature:** `undefined setCustomAutomation(Array automationData)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setCustomAutomation([{"ID": "Volume", "min": 0.0, "max": 1.0, "connections": []}]);`

**Description:**
Defines the custom automation slot layout for host and MIDI parameter mapping. Accepts an array of JSON objects, each defining one automation slot with its ID, range, value display mode, and connection targets. Each slot becomes a DAW-visible plugin parameter (if `allowHostAutomation` is true) and can be mapped to MIDI CC (if `allowMidiAutomation` is true). Connections route the slot's value to module parameters, other automation slots (meta-parameters), or global routing cables. The custom data model must be enabled via `setUseCustomUserPresetModel` before calling this method -- otherwise a script error is thrown. Each slot definition is validated during construction; if any slot has an invalid configuration (e.g., a `processorId` that does not exist), a script error is reported with the slot's ID and the specific error message.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| automationData | Array | no | Array of JSON objects, each defining one automation slot. | Must be an Array; each element must be a JSON object with at least `ID` and `connections` |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| ID | String | Unique identifier for the automation slot (required) |
| min | Double | Range minimum (default: 0.0) |
| max | Double | Range maximum (default: 1.0) |
| middlePosition | Double | If set, applies skew factor so this value appears at the center of the range |
| stepSize | Double | Quantization step size (default: 0.0 = continuous) |
| defaultValue | Double | Initial value, clamped to range (default: range minimum) |
| allowMidiAutomation | Integer | Whether MIDI CC can control this slot (default: true) |
| allowHostAutomation | Integer | Whether the DAW host can control this slot (default: true) |
| pluginParameterGroup | String | Plugin parameter group name (must be registered via `setPluginParameterGroupNames` first) |
| connections | Array | Array of connection target objects (required, can be empty) |
| mode | String | Value-to-text display mode for DAW parameter readout |
| options | Array | Array of discrete option label strings (alternative to `mode`) |
| suffix | String | Suffix for numeric text display when neither `mode` nor `options` is set |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Frequency" | Displays value as Hz/kHz (e.g., "440 Hz", "1.2 kHz") |
| "Time" | Displays value as ms/s (e.g., "100ms", "1.5s") |
| "TempoSync" | Displays value as tempo name (e.g., "1/4", "1/8T") |
| "Pan" | Displays value as L/C/R panning (e.g., "50L", "C", "30R") |
| "NormalizedPercentage" | Displays 0.0-1.0 as 0-100% |
| "Decibel" | Displays value as dB (e.g., "-6.0 dB") |
| "Semitones" | Displays value as semitones (e.g., "+2 st", "-12 st") |

**Pitfalls:**
- Throws a script error if `setUseCustomUserPresetModel` has not been called first. The custom data model is a prerequisite for custom automation.
- MetaConnection targets (referenced via `automationId`) must appear earlier in the automation array than the slot that references them. Forward references to later slots are not resolved.

**Cross References:**
- `$API.UserPresetHandler.setUseCustomUserPresetModel$`
- `$API.UserPresetHandler.attachAutomationCallback$`
- `$API.UserPresetHandler.setAutomationValue$`
- `$API.UserPresetHandler.getAutomationIndex$`
- `$API.UserPresetHandler.updateAutomationValues$`
- `$API.UserPresetHandler.setPluginParameterGroupNames$`

**DiagramRef:** automation-connection-types

**Example:**
```javascript:custom-automation-setup
// Title: Setting up custom automation with processor and cable connections
const var uph = Engine.createUserPresetHandler();

inline function onLoad(data) {};
inline function onSave(presetName) { return {}; };

uph.setUseCustomUserPresetModel(onLoad, onSave, false);

uph.setCustomAutomation([
{
    "ID": "Volume",
    "min": -100.0,
    "max": 0.0,
    "stepSize": 0.1,
    "defaultValue": -12.0,
    "mode": "Decibel",
    "connections": [
        {"processorId": "SimpleGain1", "parameterId": "Gain"}
    ]
},
{
    "ID": "FilterFreq",
    "min": 20.0,
    "max": 20000.0,
    "middlePosition": 1000.0,
    "mode": "Frequency",
    "connections": [
        {"cableId": "FilterCable"}
    ]
}]);
```
```json:testMetadata:custom-automation-setup
{
  "testable": false,
  "skipReason": "Requires a SimpleGain1 module in the module tree and a global routing cable named FilterCable to exist."
}
```

## setEnableUserPresetPreprocessing

**Signature:** `undefined setEnableUserPresetPreprocessing(Integer processBeforeLoading, Integer shouldUnpackComplexData)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setEnableUserPresetPreprocessing(true, true);`

**Description:**
Configures the preprocessing mode for user preset loading. When `processBeforeLoading` is true, the pre-callback (registered via `setPreCallback`) receives a JSON object representing the full preset data instead of a `ScriptFile` pointing to the preset file. This JSON object can be inspected and modified before the preset is loaded -- enabling version migration, value remapping, or conditional data transformation.

The JSON object passed to the pre-callback has this structure:
- `version` -- the version string from the preset
- `Content` -- array of component value objects with `id`, `value`, and other properties
- `Modules` -- module state data
- `MidiAutomation` -- MIDI automation mappings
- `MPEData` -- MPE configuration

When `shouldUnpackComplexData` is true, values that were serialized as JSON strings (prefixed with "JSON") are parsed back to JSON objects, and Base64-encoded `data` properties are decoded. This makes the preset data fully inspectable as native HISEScript objects rather than encoded strings. After the pre-callback returns, the (potentially modified) JSON is converted back to a ValueTree for the actual load.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| processBeforeLoading | Integer | no | Enable JSON preprocessing of presets in the pre-callback. | Boolean value |
| shouldUnpackComplexData | Integer | no | Decode JSON-encoded and Base64-encoded values within the preset data. | Boolean value; only meaningful when processBeforeLoading is true |

**Pitfalls:**
- The pre-callback must be set via `setPreCallback` for preprocessing to have any effect. Enabling preprocessing without a pre-callback simply adds unnecessary JSON conversion overhead on every preset load.

**Cross References:**
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.isOldVersion$`

**Diagram:**
- **Brief:** Preprocessing Data Flow
- **Type:** topology
- **Description:** Shows the two data flow paths in prePresetLoad: When preprocessing is disabled, the pre-callback receives a ScriptFile and the ValueTree passes through unchanged. When preprocessing is enabled, the ValueTree is converted to a JSON object (via convertToJson), passed to the pre-callback for potential modification, then converted back to a ValueTree (via applyJSON) for the load. The shouldUnpackComplexData flag adds an additional decoding step during convertToJson where JSON-prefixed strings and Base64 data properties are expanded.

## setParameterGestureCallback

**Signature:** `undefined setParameterGestureCallback(Function callbackFunction)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setParameterGestureCallback(onGesture);`

**Description:**
Registers a callback that fires when a DAW host begins or ends a parameter gesture (touch automation). The callback is invoked synchronously via `callSync` whenever the host calls `beginChangeGesture` or `endChangeGesture` on any registered plugin parameter. The callback receives three arguments: the parameter type (matching the `HisePluginParameterBase::Type` enum), the slot index within that type, and a boolean indicating whether the gesture is starting (true) or ending (false). This enables the plugin UI to provide visual feedback during automation recording -- for example, highlighting a knob while the user is touching it in the DAW.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackFunction | Function | yes | The callback function to invoke on gesture begin/end events. | Must accept 3 arguments |

**Callback Signature:** callbackFunction(automationType: int, slotIndex: int, startGesture: bool)

**Pitfalls:**
- [BUG] The `WeakCallbackHolder` for this callback is initialized with `numExpectedArgs=2` (in both the constructor initializer and `setParameterGestureCallback`), and the parse-time diagnostic (`ADD_CALLBACK_DIAGNOSTIC`) reports that the callback expects 2 arguments. However, the actual `onParameterGesture` implementation passes 3 arguments (type, slotIndex, startGesture) via `callSync(NativeFunctionArgs)`. A user following the diagnostic guidance would write a 2-parameter callback and silently miss the `startGesture` boolean. The callback must accept 3 arguments to receive all data.

**Cross References:**
- `$API.UserPresetHandler.sendParameterGesture$`
- `$API.UserPresetHandler.setCustomAutomation$`

**Example:**
```javascript:gesture-callback-setup
// Title: Tracking DAW parameter gesture begin/end events
const var uph = Engine.createUserPresetHandler();

inline function onGesture(type, slotIndex, isStart)
{
    Console.print("Gesture " + (isStart ? "begin" : "end") +
                  " type=" + type + " slot=" + slotIndex);
};

uph.setParameterGestureCallback(onGesture);
```
```json:testMetadata:gesture-callback-setup
{
  "testable": false,
  "skipReason": "Requires a DAW host to trigger parameter gesture events on registered plugin parameters."
}
```

## setPluginParameterGroupNames

**Signature:** `undefined setPluginParameterGroupNames(Array pluginParameterGroupNames)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setPluginParameterGroupNames(["Oscillators", "Filters", "Effects"]);`

**Description:**
Registers the valid set of plugin parameter group names that can be used in `setCustomAutomation` slot definitions (via the `pluginParameterGroup` property). The names are stored as a `StringArray` on the `UserPresetHandler` and validated when `setCustomAutomation` parses each slot's `pluginParameterGroup` field -- if a slot references a group name that was not registered here, a validation error is reported. An empty string is always valid as a group name (it is the default). This method must be called before `setCustomAutomation` if any automation slots use non-empty `pluginParameterGroup` values. Plugin parameter groups organize DAW-visible parameters into named categories, which some hosts display as submenus or folders.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pluginParameterGroupNames | Array | no | Array of string values representing valid group names. Each element is converted to a string via `toString()`. | Must be an Array; throws a script error if not |

**Pitfalls:**
- Passing a non-array value (e.g., a single string) throws a script error with message "pluginParameterGroupNames must be an array of strings". This is the only validation -- individual elements are not checked for type; they are silently converted to strings via `toString()`.

**Cross References:**
- `$API.UserPresetHandler.setCustomAutomation$`
- `$API.UserPresetHandler.setPluginParameterSortFunction$`

## setPluginParameterSortFunction

**Signature:** `undefined setPluginParameterSortFunction(Function customSortFunction)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setPluginParameterSortFunction(onSortParams);`

**Description:**
Installs a custom sort function that determines the order of plugin parameters as exposed to the DAW host. The sort function receives two JSON objects representing plugin parameters and must return a negative integer (first before second), zero (equal), or a positive integer (second before first). If the callback returns `undefined` or `void`, the default sorting order is used for that pair. Passing a non-function value (e.g., `false`) resets to the default sort behavior (`HisePluginParameterBase::defaultSort`). The sort function is stored on `PluginParameterAudioProcessor` and invoked whenever the host queries the plugin parameter list. The callback is called synchronously via `callSync`, so it executes on whatever thread the host queries parameters from.

Each of the two parameter objects passed to the callback has these properties:

| Property | Type | Description |
|----------|------|-------------|
| type | Integer | The parameter type: 0=Macro, 1=CustomAutomation, 2=ScriptControl, 3=NKSWrapper |
| parameterIndex | Integer | The HISE-internal parameter index |
| typeIndex | Integer | The slot index within the parameter's type category |
| name | String | The parameter's display name |
| group | String | The plugin parameter group name (empty string if no group) |

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| customSortFunction | Function | no | Sort comparison function. Pass a non-function to reset to default sorting. | Must accept 2 arguments when a function; must return Integer |

**Callback Signature:** customSortFunction(a: Object, b: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| type | Integer | Parameter type enum: 0=Macro, 1=CustomAutomation, 2=ScriptControl, 3=NKSWrapper |
| parameterIndex | Integer | HISE-internal parameter index |
| typeIndex | Integer | Slot index within the parameter's type category |
| name | String | Parameter display name |
| group | String | Plugin parameter group name (empty string if ungrouped) |

**Cross References:**
- `$API.UserPresetHandler.setPluginParameterGroupNames$`
- `$API.UserPresetHandler.setCustomAutomation$`

**Example:**
```javascript:sort-params-by-group
// Title: Sorting plugin parameters by group name then by slot index
const var uph = Engine.createUserPresetHandler();

inline function onSortParams(a, b)
{
    // Sort by group name first, then by slot index within the group
    if (a.group < b.group)
        return -1;
    if (a.group > b.group)
        return 1;
    return a.typeIndex - b.typeIndex;
};

uph.setPluginParameterSortFunction(onSortParams);
```
```json:testMetadata:sort-params-by-group
{
  "testable": false,
  "skipReason": "Requires registered plugin parameters (via setCustomAutomation) and a DAW host context to observe the sort order."
}
```

## setPostCallback

**Signature:** `undefined setPostCallback(Function presetPostCallback)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setPostCallback(onPresetLoaded);`

**Description:**
Registers a callback that fires after a user preset has been loaded. The callback executes asynchronously on the message thread -- it is dispatched via `callOnMessageThreadAfterSuspension` after the entire background load completes, including macro restoration, module state recovery, MIDI automation, and MPE data. The callback receives one argument: a `ScriptFile` object pointing to the loaded preset file. If the preset was loaded from a non-file source (e.g., a DAW state restore where no physical file exists), the argument may be `undefined` (the `ScriptFile` is only created when `newPreset.existsAsFile()` is true). This callback is the correct place to update UI state after a preset change -- all component values and module states are fully restored by the time it fires.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| presetPostCallback | Function | yes | Callback to invoke after preset load completes on the message thread. | Must accept 1 argument |

**Callback Signature:** presetPostCallback(presetFile: ScriptFile)

**Pitfalls:**
- The callback argument is `undefined` (not a ScriptFile) when the preset was loaded from a non-file source such as a DAW session restore. Code that unconditionally calls methods on the argument (e.g., `presetFile.toString("")`) will produce an error in this case. Guard with `isDefined(presetFile)` or `typeof presetFile == "object"`.

**Cross References:**
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setPostSaveCallback$`
- `$API.UserPresetHandler.isInternalPresetLoad$`
- `$API.UserPresetHandler.isCurrentlyLoadingPreset$`

**DiagramRef:** preset-load-sequence

**Example:**
```javascript:post-callback-ui-update
// Title: Updating UI state after a preset is loaded
const var uph = Engine.createUserPresetHandler();

uph.setPostCallback(function(presetFile)
{
    if (isDefined(presetFile))
        Console.print("Loaded preset: " + presetFile.toString(""));
    else
        Console.print("Preset loaded from DAW state (no file)");
});
```
```json:testMetadata:post-callback-ui-update
{
  "testable": false,
  "skipReason": "Requires a preset load to trigger the callback; no programmatic trigger available from onInit."
}
```

## setPostSaveCallback

**Signature:** `undefined setPostSaveCallback(Function presetPostSaveCallback)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setPostSaveCallback(onPresetSaved);`

**Description:**
Registers a callback that fires after a user preset has been saved. The callback executes on the message thread (the save path calls `postPresetSave` which asserts `isThisTheMessageThread()`). The callback receives one argument: a `ScriptFile` object pointing to the saved preset file, or `undefined` if the file does not exist (e.g., save to a non-file target). This is useful for updating the UI after a save operation, such as refreshing a preset browser or confirming the save to the user.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| presetPostSaveCallback | Function | yes | Callback to invoke after a preset has been saved. | Must accept 1 argument |

**Callback Signature:** presetPostSaveCallback(presetFile: ScriptFile)

**Pitfalls:**
- The callback argument is `undefined` when the preset was saved to a non-file target. Guard with `isDefined(presetFile)` before calling methods on it.

**Cross References:**
- `$API.UserPresetHandler.setPostCallback$`
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setUseCustomUserPresetModel$`

**Example:**
```javascript:post-save-notification
// Title: Logging preset save events
const var uph = Engine.createUserPresetHandler();

uph.setPostSaveCallback(function(presetFile)
{
    if (isDefined(presetFile))
        Console.print("Saved preset to: " + presetFile.toString(""));
});
```
```json:testMetadata:post-save-notification
{
  "testable": false,
  "skipReason": "Requires a preset save operation to trigger the callback; no programmatic trigger available from onInit."
}
```

## setPreCallback

**Signature:** `undefined setPreCallback(Function presetPreCallback)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setPreCallback(onPresetPreLoad);`

**Description:**
Registers a callback that fires synchronously before a user preset is loaded. The callback executes on the calling thread before voices are killed and the loading thread is entered. This is the correct place to implement preset migration logic, version checks, or data transformations.

The callback argument depends on whether preprocessing is enabled:

- **Without preprocessing** (default): The callback receives a `ScriptFile` pointing to the preset file. The preset data passes through to the load unchanged regardless of what the callback does.
- **With preprocessing** (`setEnableUserPresetPreprocessing(true, ...)`): The callback receives a JSON object representing the full preset data. The callback can modify this object in-place (e.g., adding/removing/changing component values, adjusting module state). After the callback returns, the modified JSON is converted back to a ValueTree for the actual load.

The callback is invoked via `callSync`, so it blocks the preset load pipeline until it returns. Long-running operations in the callback will delay the entire preset load.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| presetPreCallback | Function | yes | Callback to invoke before preset loading begins. | Must accept 1 argument |

**Callback Signature:** presetPreCallback(presetData: var)

**Cross References:**
- `$API.UserPresetHandler.setPostCallback$`
- `$API.UserPresetHandler.setEnableUserPresetPreprocessing$`
- `$API.UserPresetHandler.isOldVersion$`
- `$API.UserPresetHandler.isInternalPresetLoad$`

**DiagramRef:** preset-load-sequence

**Example:**
```javascript:pre-callback-version-check
// Title: Version-aware preset migration in the pre-callback
const var uph = Engine.createUserPresetHandler();
uph.setEnableUserPresetPreprocessing(true, false);

uph.setPreCallback(function(presetData)
{
    if (uph.isOldVersion(presetData.version))
    {
        // Example: rename a component that was changed in v1.1.0
        for (c in presetData.Content)
        {
            if (c.id == "OldKnobName")
                c.id = "NewKnobName";
        }
    }
});
```
```json:testMetadata:pre-callback-version-check
{
  "testable": false,
  "skipReason": "Requires a preset load with an actual preset file containing version data to trigger the pre-callback."
}
```

## setUseCustomUserPresetModel

**Signature:** `undefined setUseCustomUserPresetModel(Function loadCallback, Function saveCallback, Integer usePersistentObject)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setUseCustomUserPresetModel(onLoad, onSave, false);`

**Description:**
Switches from the default user preset data model (which automatically serializes all `saveInPreset` component values as a ValueTree) to a custom data model where script callbacks handle save and load. Both callbacks must be valid JavaScript functions -- if either is not a function, the method silently does nothing (no error, no mode change). When the custom model is active, preset saves call the save callback to obtain a JSON object, and preset loads pass a JSON object to the load callback. This must be called before `setCustomAutomation`, which requires the custom data model as a prerequisite.

The **load callback** receives one argument: a `var` containing the custom data that was saved with the preset. This is called synchronously on the loading thread with the script lock held. The data is a JSON object that was previously returned by the save callback and stored in the preset's `CustomJSON` state segment.

The **save callback** receives one argument: a `String` containing the preset name (currently always "Unused" from the `CustomStateManager`, but the listener interface provides the actual preset name when called from other paths). The callback must return a JSON object representing the data to be stored. This is called synchronously on the message thread with the script lock held.

The `usePersistentObject` flag is stored and controls whether the custom data persists between preset saves within the broader preset system.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| loadCallback | Function | no | Callback invoked during preset load to restore custom state. | Must be a JavaScript function; must accept 1 argument |
| saveCallback | Function | no | Callback invoked during preset save to serialize custom state. Must return a JSON object. | Must be a JavaScript function; must accept 1 argument |
| usePersistentObject | Integer | no | Whether the custom data persists between preset saves. | Boolean value |

**Callback Signature:** loadCallback(data: var)
**Callback Signature:** saveCallback(presetName: String)

**Pitfalls:**
- [BUG] If either `loadCallback` or `saveCallback` is not a valid JavaScript function, the method silently returns without enabling the custom model. No error is thrown, so the user may believe the custom model is active when it is not. Subsequent calls to `setCustomAutomation` will then fail with "you need to enable setUseCustomDataModel() before calling this method".

**Cross References:**
- `$API.UserPresetHandler.setCustomAutomation$`
- `$API.UserPresetHandler.createObjectForSaveInPresetComponents$`
- `$API.UserPresetHandler.updateSaveInPresetComponents$`
- `$API.UserPresetHandler.runTest$`

**Example:**
```javascript:custom-data-model-setup
// Title: Setting up a custom user preset data model
const var uph = Engine.createUserPresetHandler();

inline function onLoad(data)
{
    if (isDefined(data))
    {
        // Restore custom state from the preset data
        Console.print("Loading custom data: " + trace(data));
    }
};

inline function onSave(presetName)
{
    // Return the custom state to be saved with the preset
    return {
        "version": "1.0.0",
        "customSetting": 42
    };
};

uph.setUseCustomUserPresetModel(onLoad, onSave, false);
```
```json:testMetadata:custom-data-model-setup
{
  "testable": false,
  "skipReason": "Requires a preset save/load cycle to exercise the custom callbacks; no programmatic trigger available from onInit."
}
```

## setUseUndoForPresetLoading

**Signature:** `undefined setUseUndoForPresetLoading(Integer shouldUseUndoManager)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setUseUndoForPresetLoading(true);`

**Description:**
Enables or disables undo support for user preset loading. When enabled, each preset load is wrapped in an `UndoableUserPresetLoad` action and pushed onto the control undo manager, allowing `Engine.undo()` to restore the previous preset state. When disabled (the default), preset loads are final and cannot be undone. The implementation simply sets a boolean flag (`useUndoForPresetLoads`) on the internal `UserPresetHandler` -- the actual undo wrapping happens in `loadUserPresetFromValueTree` which checks this flag before deciding whether to create an undoable action or load directly. Consecutive undoable preset loads are coalesced: the `UndoableUserPresetLoad` action implements `createCoalescedAction` to keep the first old state while taking the last new state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseUndoManager | Integer | no | Whether to wrap preset loads in undoable actions. | Boolean value |

**Cross References:**
- `$API.UserPresetHandler.resetToDefaultUserPreset$`
- `$API.UserPresetHandler.updateAutomationValues$`
- `$API.UserPresetHandler.setPreCallback$`
- `$API.UserPresetHandler.setPostCallback$`
- `$API.Engine.undo$`

## updateAutomationValues

**Signature:** `undefined updateAutomationValues(var data, var sendMessage, Integer useUndoManager)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Heap-allocates DynamicObjects for the IndexSorter, Array::sort, and undo action path; calls CustomAutomationData::call which dispatches through connections.
**Minimal Example:** `{obj}.updateAutomationValues([{"id": "Volume", "value": 0.5}], SyncNotification, false);`

**Description:**
Updates custom automation values from either an array of value objects or an integer connection index. This method supports two distinct input modes:

**Mode 1 -- Integer input:** When `data` is an integer, it is interpreted as a `preferredProcessorIndex`. The method iterates all custom automation slots and calls `updateFromConnectionValue(preferredProcessorIndex)` on each, which reads back the current value from the slot's processor connection at that index and updates the automation slot to match. No explicit values are set -- this is a "sync from processors" operation. The `sendMessage` parameter is ignored in this mode (the dispatch type is always `sendNotificationSync` from the internal `updateFromConnectionValue` call).

**Mode 2 -- Array of objects input:** When `data` is an array, each element must be a JSON object with `id` (string matching a registered automation ID) and `value` (numeric value to set). The array is sorted by automation index before applying values to ensure consistent ordering. For each element, the value is sanitized, and `CustomAutomationData::call` is invoked with the dispatch type derived from `sendMessage`.

The `sendMessage` parameter uses dispatch type encoding: pass `SyncNotification` for synchronous dispatch, `AsyncNotification` for asynchronous, or `false` for `dontSendNotification` (update values without triggering callbacks). When `useUndoManager` is true, the operation is wrapped in an `AutomationValueUndoAction` and pushed onto the control undo manager for `Engine.undo()` support.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| data | var | no | Either an Integer (preferred processor connection index) or an Array of `{"id": "...", "value": ...}` objects. | Integer or Array; passing a single JSON object throws a script error |
| sendMessage | var | no | Dispatch type for value change notifications. | `SyncNotification`, `AsyncNotification`, `AsyncHiPriorityNotification`, or `false` (dontSendNotification) |
| useUndoManager | Integer | no | Whether to wrap the value changes in an undoable action. | Boolean value |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| id | String | The automation slot identifier matching the `ID` field from `setCustomAutomation` |
| value | Double | The value to set on the automation slot |

**Pitfalls:**
- Passing a single JSON object (not wrapped in an array) throws a script error. Always wrap value objects in an array, even for a single slot update.
- [BUG] The internal IndexSorter has a copy-paste bug: both `i1` and `i2` are constructed from `first["id"]` instead of `i2` using `second["id"]`. This causes the sort comparator to always return 0 (equal), effectively disabling the index-based sort. Values are applied in array order rather than automation index order.
- [BUG] The undo path (`useUndoManager=true`) does not capture old values for array inputs. The `AutomationValueUndoAction` constructor attempts to capture old values via `newData.getDynamicObject()`, which returns null for array inputs. The undo action's `oldData` remains undefined, so `Engine.undo()` silently does nothing instead of restoring previous values.

**Cross References:**
- `$API.UserPresetHandler.createObjectForAutomationValues$`
- `$API.UserPresetHandler.setAutomationValue$`
- `$API.UserPresetHandler.setCustomAutomation$`
- `$API.UserPresetHandler.attachAutomationCallback$`

**Example:**
```javascript:update-automation-batch
// Title: Batch-updating automation values with array input
const var uph = Engine.createUserPresetHandler();

// After custom automation has been set up:
// Batch-update multiple automation values at once
uph.updateAutomationValues([
    {"id": "Volume", "value": 0.75},
    {"id": "Pan", "value": 0.5}
], SyncNotification, false);

// Mode 1: Refresh all slots from their processor connections
uph.updateAutomationValues(0, SyncNotification, false);
```
```json:testMetadata:update-automation-batch
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with full automation data setup and connected processors prior to calling."
}
```

## updateConnectedComponentsFromModuleState

**Signature:** `undefined updateConnectedComponentsFromModuleState()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Iterates all UI components, calls `updateValueFromProcessorConnection()` on each which reads processor attributes and calls `setValue()` -- involves floating-point sanitization and value change notifications.
**Minimal Example:** `{obj}.updateConnectedComponentsFromModuleState();`

**Description:**
Refreshes the values of all UI components that are connected to processor parameters via the `processorId`/`parameterId` properties. The method iterates every component in the scripting content and calls `updateValueFromProcessorConnection()` on each. For components with a valid processor connection, this reads the current parameter value from the connected processor (via `getAttribute`, `getIntensity`, or `isBypassed` depending on the connection type) and calls `setValue()` to update the component. Components without a processor connection are unaffected (the method checks for a non-null `connectedProcessor` and valid `connectedParameterIndex`).

This is useful after programmatically changing module parameters (e.g., via `setAttribute` or module state restoration) to synchronize the UI with the new parameter values. It is the inverse direction of normal control flow: instead of UI driving parameters, parameters drive UI.

The processor connection types that are refreshed:

| connectedParameterIndex | Connection Type | Value Source |
|------------------------|----------------|--------------|
| >= 0 | Normal parameter | `processor.getAttribute(index)` |
| -2 | Modulation intensity | `modulator.getIntensity()` |
| -3 | Bypass (inverted) | `processor.isBypassed() ? 1.0 : 0.0` |
| -4 | Bypass (normal) | `processor.isBypassed() ? 0.0 : 1.0` |

**Parameters:**

(none)

**Cross References:**
- `$API.UserPresetHandler.updateSaveInPresetComponents$`
- `$API.UserPresetHandler.createObjectForSaveInPresetComponents$`
- `$API.UserPresetHandler.setUseCustomUserPresetModel$`

## updateSaveInPresetComponents

**Signature:** `undefined updateSaveInPresetComponents(JSON obj)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Converts a DynamicObject to a ValueTree, iterates components for type lookup, and calls restoreAllControlsFromPreset which triggers value change notifications and potential UI repaints.
**Minimal Example:** `{obj}.updateSaveInPresetComponents(componentData);`

**Description:**
Restores UI component values from a JSON object that was previously created by `createObjectForSaveInPresetComponents`. The method converts the JSON object back to a ValueTree, then re-adds the `type` property to each child element by looking up the component by its `id` in the scripting content. Finally, it calls `restoreAllControlsFromPreset` to apply the values to all matching components. This is the write half of the round-trip pair with `createObjectForSaveInPresetComponents`.

This method is primarily used within the custom data model's load callback to restore component state from custom preset data. It allows selective restoration of `saveInPreset` component values as part of a custom save/load scheme.

The `type` property is stripped during `createObjectForSaveInPresetComponents` (because the ValueTree-to-JSON conversion drops it) and re-added here by looking up each component's actual type. If a component ID in the data does not match any existing component, the entry is silently skipped (the `getComponentWithName` check returns null and the type is not added, but `restoreAllControlsFromPreset` still processes the entry -- it simply will not match any component).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | JSON | no | A JSON object previously returned by `createObjectForSaveInPresetComponents`, containing component id/value pairs. | Must be a valid JSON object with the structure produced by `createObjectForSaveInPresetComponents` |

**Cross References:**
- `$API.UserPresetHandler.createObjectForSaveInPresetComponents$`
- `$API.UserPresetHandler.setUseCustomUserPresetModel$`

**Example:**
```javascript:save-restore-component-state
// Title: Round-trip save and restore of component values in custom data model
const var uph = Engine.createUserPresetHandler();

inline function onLoad(data)
{
    if (isDefined(data) && isDefined(data.components))
    {
        // Restore the component values from the custom preset data
        uph.updateSaveInPresetComponents(data.components);
    }
};

inline function onSave(presetName)
{
    // Capture component values and include them in the custom preset data
    return {
        "components": uph.createObjectForSaveInPresetComponents(),
        "customSettings": {"reverb": 0.5}
    };
};

uph.setUseCustomUserPresetModel(onLoad, onSave, false);
```
```json:testMetadata:save-restore-component-state
{
  "testable": false,
  "skipReason": "Requires a preset save/load cycle to exercise the custom data model callbacks; no programmatic trigger available from onInit."
}
```
