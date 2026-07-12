# Engine -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- No prerequisite for Engine
- `enrichment/resources/survey/class_survey_data.json` -- Engine entry (line 699)
- `enrichment/base/Engine.json` -- 147 methods

## Source Locations
- **Header:** `HISE/hi_scripting/scripting/api/ScriptingApi.h` lines 236-715
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 1153-3733
- Engine is a nested class inside `ScriptingApi`

---

## Class Declaration

```cpp
class Engine: public ScriptingObject,
              public ApiClass
```

Engine inherits from:
- `ScriptingObject` -- provides `getProcessor()`, `getScriptProcessor()`, `reportScriptError()`, `debugToConsole()`
- `ApiClass` -- provides the method registration infrastructure (`addFunction`, `addConstant`, `addForcedParameterTypes`), constant resolution, and diagnostic hooks

Engine is a **namespace-style** API class (not a `ConstScriptingObject`) -- it is instantiated once per script processor and accessed as the global `Engine` object. The category is `"namespace"` in the base JSON.

## Member Variables

```cpp
double unused = 0.0;                           // dead field
ScriptBaseMidiProcessor* parentMidiProcessor;   // cached dynamic_cast for getUptime() timestamp precision
ScopedPointer<Thread> currentExportThread;      // holds AudioRenderer for renderAudio()
ScopedPointer<PreviewHandler> previewHandler;   // lazy-created for playBuffer()
```

## Constructor & Destructor

Constructor: `Engine(ProcessorWithScriptingContent *p)`
- Calls `ScriptingObject(p)` and `ApiClass(0)` (zero constants -- Engine has no `addConstant()` calls)
- Caches `parentMidiProcessor = dynamic_cast<ScriptBaseMidiProcessor*>(p)`
- Registers all 147 methods via `ADD_API_METHOD_N()` and `ADD_TYPED_API_METHOD_N()` macros

Destructor: Empty (just `{}`).

**No constants are registered.** This is notable -- Engine exposes no `addConstant()` values. All constant-like data is returned via methods (`getOS()`, `getFilterModeList()`, etc.).

---

## Method Registration Infrastructure

### Wrapper Struct Pattern

All methods follow the HISE two-layer registration pattern:

1. **Static wrapper functions** in `struct Engine::Wrapper` (lines 1153-1303) using macros:
   - `API_VOID_METHOD_WRAPPER_N(Engine, methodName)` -- for void returns
   - `API_METHOD_WRAPPER_N(Engine, methodName)` -- for value returns
   These generate static functions that cast `ApiClass*` to `Engine*` and call the method.

2. **Registration in constructor** using macros:
   - `ADD_API_METHOD_N(methodName)` -- creates Identifier, calls `addFunctionN(id, &Wrapper::methodName)`
   - `ADD_TYPED_API_METHOD_N(methodName, types...)` -- same as above PLUS calls `addForcedParameterTypes()` for compile-time type checking

### Typed (Forced) Parameter Methods

These methods have forced parameter types (checked at compile time in `ENABLE_SCRIPTING_SAFE_CHECKS` builds):

| Method | Param 1 Type | Param 2 Type |
|--------|-------------|-------------|
| `setHostBpm` | Number | -- |
| `getMilliSecondsForTempo` | Number | -- |
| `getSamplesForMilliSeconds` | Number | -- |
| `getMilliSecondsForSamples` | Number | -- |
| `getQuarterBeatsForMilliSeconds` | Number | -- |
| `getQuarterBeatsForSamples` | Number | -- |
| `getSamplesForQuarterBeats` | Number | -- |
| `getMilliSecondsForQuarterBeats` | Number | -- |
| `getGainFactorForDecibels` | Number | -- |
| `getDecibelsForGainFactor` | Number | -- |
| `getFrequencyForMidiNoteNumber` | Number | -- |
| `getPitchRatioFromSemitones` | Number | -- |
| `getSemitonesFromPitchRatio` | Number | -- |
| `setMinimumSampleRate` | Number | -- |
| `setMaximumBlockSize` | Number | -- |
| `performUndoAction` | JSON | Function |

All other methods use the plain `ADD_API_METHOD_N` macro and rely on runtime type coercion.

---

## Method Groupings

The 147 methods naturally cluster into these functional groups:

### Group 1: Object Factories (27 methods)

Methods that create scripting API objects. These all return `var` or typed pointers and are called from `onInit`.

| Method | Returns | Preprocessor Guard |
|--------|---------|-------------------|
| `createBroadcaster` | `ScriptBroadcaster` | -- |
| `createTimerObject` | `TimerObject` | -- |
| `createMidiList` | `MidiList` | -- |
| `createUnorderedStack` | `ScriptUnorderedStack` | -- |
| `createMessageHolder` | `ScriptingMessageHolder` | -- |
| `createBackgroundTask` | `ScriptBackgroundTask` | -- |
| `createDspNetwork` | `DspNetwork` (via `Holder::getOrCreate`) | -- |
| `createExpansionHandler` | `ScriptExpansionHandler` | -- |
| `createUserPresetHandler` | `ScriptUserPresetHandler` | -- |
| `createMidiAutomationHandler` | `ScriptedMidiAutomationHandler` | -- |
| `createMacroHandler` | `ScriptedMacroHandler` | -- |
| `createErrorHandler` | `ScriptErrorHandler` | -- |
| `createTransportHandler` | `TransportHandler` | -- |
| `createGlobalScriptLookAndFeel` | `ScriptedLookAndFeel` (singleton pattern) | -- |
| `createFFT` | `ScriptFFT` | -- |
| `createFixObjectFactory` | `fixobj::Factory` | -- |
| `createThreadSafeStorage` | `ScriptThreadSafeStorage` | -- |
| `createLicenseUnlocker` | `ScriptUnlocker::RefObject` | -- |
| `createBeatportManager` | `BeatportManager` | -- |
| `createNeuralNetwork` | `ScriptNeuralNetwork` | -- |
| `createModulationMatrix` | `ScriptModulationMatrix` | -- |
| `createBXLicenser` | `ScriptBXLicenser` | `HISE_INCLUDE_BX_LICENSER` |
| `createNKSManager` | `ScriptNKSManager` | `HISE_INCLUDE_NKS_SDK` |
| `createAndRegisterSliderPackData` | `ScriptSliderPackData` | -- |
| `createAndRegisterTableData` | `ScriptTableData` | -- |
| `createAndRegisterAudioFile` | `ScriptAudioFile` | -- |
| `createAndRegisterRingBuffer` | `ScriptRingBuffer` | -- |

**Factory Pattern Notes:**
- Most factories just `new` the object with `getScriptProcessor()` as first argument
- `createGlobalScriptLookAndFeel()` is a **singleton-or-create** pattern -- checks `mc->getCurrentScriptLookAndFeel()` first, returns existing if found, else creates new with `isGlobal=true`
- `createDspNetwork()` requires the processor to be a `scriptnode::DspNetwork::Holder` -- reports error if not
- `createBXLicenser()` and `createNKSManager()` are guarded by preprocessor flags and report errors if the SDK is not enabled
- `createAndRegister*` methods take an `int index` parameter for slot-based registration

### Group 2: Reference Accessors (4 methods)

Methods that look up existing objects rather than creating new ones:

| Method | Looks Up |
|--------|----------|
| `getDspNetworkReference` | DspNetwork from another processor by processorId + networkId |
| `getGlobalRoutingManager` | Creates new `GlobalRoutingManagerReference` wrapper each call |
| `getLorisManager` | Creates new `ScriptLorisManager` wrapper, guarded by `HISE_INCLUDE_LORIS` |
| `getComplexDataReference` | Table/SliderPack/AudioFile/DisplayBuffer from any `ExternalDataHolder` module |

**`getComplexDataReference` details:**
- `dataType` parameter must be one of: `"Table"`, `"SliderPack"`, `"AudioFile"`, `"FilterCoefficients"`, `"DisplayBuffer"`
- Looks up processor by name via `ProcessorHelpers::getFirstProcessorWithName()`
- Casts to `ExternalDataHolder*`, retrieves complex data by type+index
- Returns `undefined` (not error) if data slot doesn't exist
- Reports error if module not found or data type is invalid

### Group 3: Unit Conversion -- Audio/Time (16 methods)

Pure math/conversion with no side effects. Most are inline in the header.

**Sample Rate based:**
- `getSamplesForMilliSeconds(ms)` / `getMilliSecondsForSamples(samples)` -- uses `getSampleRate()`

**Tempo based (current host BPM):**
- `getQuarterBeatsForSamples(samples)` / `getSamplesForQuarterBeats(quarterBeats)`
- `getQuarterBeatsForMilliSeconds(ms)` / `getMilliSecondsForQuarterBeats(quarterBeats)`

**Tempo based (explicit BPM):**
- `getQuarterBeatsForSamplesWithTempo(samples, bpm)` / `getSamplesForQuarterBeatsWithTempo(quarterBeats, bpm)`
- `getQuarterBeatsForMilliSecondsWithTempo(ms, bpm)` / `getMilliSecondsForQuarterBeatsWithTempo(quarterBeats, bpm)`

**Tempo sync:**
- `getMilliSecondsForTempo(tempoIndex)` -- uses `TempoSyncer::getTempoInMilliSeconds()`
- `getTempoName(tempoIndex)` -- returns human-readable name like "1/4", "1/8T"

All tempo-based methods delegate to `TempoSyncer` utility class.

### Group 4: Unit Conversion -- Musical (6 methods)

- `getGainFactorForDecibels(dB)` / `getDecibelsForGainFactor(gain)` -- `Decibels::decibelsToGain/gainToDecibels`
- `getFrequencyForMidiNoteNumber(note)` -- `MidiMessage::getMidiNoteInHertz()`
- `getPitchRatioFromSemitones(st)` / `getSemitonesFromPitchRatio(ratio)` -- `pow(2, st/12)` / `1200*log2(ratio)`
- `getMidiNoteName(note)` / `getMidiNoteFromName(name)` -- MIDI note <-> string ("C3" etc.)

### Group 5: Audio Engine Control (8 methods)

- `getSampleRate()` / `getBufferSize()` / `getNumPluginChannels()` -- read current audio config
- `setMinimumSampleRate(rate)` -- sets oversampling floor
- `setMaximumBlockSize(blockSize)` -- splits buffers if host block > this
- `getControlRateDownsamplingFactor()` -- returns `HISE_EVENT_RASTER` (compile-time constant, default 8)
- `setLatencySamples(n)` / `getLatencySamples()` -- wraps `AudioProcessor::setLatencySamples()`

### Group 6: Global State (9 methods)

- `getHostBpm()` / `setHostBpm(bpm)` -- reads/writes `GlobalSettingManager::globalBPM`. -1 = sync to host.
- `getUptime()` -- engine uptime in seconds. If called from MIDI callback, includes event timestamp offset for sub-buffer accuracy.
- `getCpuUsage()` / `getMemoryUsage()` / `getNumVoices()` -- performance monitoring
- `setGlobalPitchFactor(semitones)` / `getGlobalPitchFactor()` -- clamped to [-12, 12]
- `allNotesOff()` -- sends all notes off at next buffer

### Group 7: User Preset Management (11 methods)

- `loadUserPreset(file)` -- loads by relative path or `ScriptFile` object. **Rejects calls during initialization** (`reportScriptError("Do not load user presets at startup.")`)
- `saveUserPreset(name)` -- accepts string name or `ScriptFile` object
- `loadNextUserPreset(stayInDir)` / `loadPreviousUserPreset(stayInDir)` -- increment/decrement
- `getCurrentUserPresetName()` -- returns filename without extension
- `getUserPresetList()` -- returns array of relative paths (without .preset extension, `/` separated)
- `isUserPresetReadOnly(optionalFile)` -- backend: checks project setting. Frontend: checks against `READ_ONLY_FACTORY_PRESETS` define.
- `setUserPresetTagList(tags)` -- sets tag list for preset browser filtering
- `addModuleStateToUserPreset(moduleId)` -- adds an entire module's state to user preset system. Passing empty string clears all.
- `setFrontendMacros(nameList)` -- enables macro system with given names. Uses `HISE_NUM_MACROS` preprocessor.
- `getMacroName(index)` -- 1-based index (1-8)

### Group 8: Expansion Management (3 methods)

- `getExpansionList()` -- creates temporary ExpansionHandler, calls `getExpansionList()` on it
- `setCurrentExpansion(name)` -- delegates to `ExpansionHandler::setCurrentExpansion()`
- `getWavetableList()` -- requires at least one `WavetableSynth` in the chain. Returns string list.

### Group 9: Pool & Resource Management (8 methods)

- `loadAudioFilesIntoPool()` -- backend: loads all audio files. Both targets: returns list of references.
- `loadAudioFileIntoBufferArray(ref)` -- loads audio file, returns array of Buffer objects (one per channel)
- `loadImageIntoPool(id)` -- backend only. Supports wildcard patterns (`*`). No-op in compiled plugins.
- `clearSampleMapPool()` / `clearMidiFilePool()` -- backend only. Clears cached pool entries.
- `rebuildCachedPools()` -- backend only. Clears and reloads MIDI file and sample map pools.
- `getSampleFilesFromDirectory(path, recursive)` -- backend only. Returns audio file references from Samples subfolder.
- `setAllowDuplicateSamples(bool)` -- controls duplicate sample detection in sample pool

### Group 10: Font Management (3 methods)

- `loadFont(fileName)` -- **DEPRECATED.** Emits `debugError` warning, delegates to `loadFontAs(fileName, "")`.
- `loadFontAs(fileName, fontId)` -- backend: reads from Images folder, calls `loadTypeFace()`. Frontend: no-op (loaded at startup).
- `setGlobalFont(fontName)` -- sets the default font used throughout the plugin

### Group 11: JSON & Data Serialization (5 methods)

- `dumpAsJSON(object, fileName)` -- writes to UserPresets subdirectory (or absolute path)
- `loadFromJSON(fileName)` -- reads from UserPresets subdirectory (or absolute path)
- `compressJSON(object)` -- JSON -> zstd compressed -> Base64 string
- `uncompressJSON(b64)` -- Base64 -> zstd decompress -> JSON parse
- `decodeBase64ValueTree(b64Data)` -- tries multiple decode strategies (ValueTreeConverters, zstd, raw MemoryBlock), returns XML string

### Group 12: String & Formatting (8 methods)

- `doubleToString(value, digits)` -- `String(value, digits)`
- `getStringWidth(text, fontName, fontSize, fontSpacing)` -- uses embedded font metrics
- `intToHexString(value)` -- `String::toHexString()`
- `matchesRegex(str, regex)` / `getRegexMatches(str, regex)` -- std::regex with error handling
- `getTextForValue(value, mode)` / `getValueForText(text, mode)` -- ValueToTextConverter with modes
- `sortWithFunction(array, fn)` -- sorts array using custom comparator via `callExternalFunctionRaw`

### Group 13: System Information (8 methods)

- `getOS()` -- compile-time: returns `"WIN"`, `"LINUX"`, or `"OSX"`
- `getSystemStats()` -- returns DynamicObject with ~14 system properties (OS name, CPU, RAM, isDarkMode)
- `getDeviceType()` -- returns `"Desktop"`, `"iPad"`, `"iPadAUv3"`, `"iPhone"`, `"iPhoneAUv3"`
- `getDeviceResolution()` -- returns `[x, y, width, height]` array
- `isPlugin()` -- `false` if `IS_STANDALONE_APP`, `true` otherwise
- `isHISE()` -- `true` if `USE_BACKEND`
- `getVersion()` / `getName()` -- from project settings (backend) or `FrontendHandler` (frontend)
- `getProjectInfo()` -- returns DynamicObject with Company, CompanyURL, CompanyCopyright, ProjectName, ProjectVersion, EncryptionKey, HISEBuild, BuildDate, LicensedEmail

### Group 14: UI & Display (6 methods)

- `setKeyColour(keyNumber, colourAsHex)` -- sets keyboard colour
- `setLowestKeyToDisplay(keyNumber)` -- scrolls on-screen keyboard
- `getZoomLevel()` / `setZoomLevel(level)` -- **DEPRECATED** (calls `logSettingWarning`). Delegates to `GlobalSettingManager`. Clamped [0.25, 2.0].
- `setDiskMode(mode)` -- **DEPRECATED** (calls `logSettingWarning`). 0=SSD, 1=HDD.
- `getMasterPeakLevel(channel)` -- 0=left, 1=right. Currently stereo only.

### Group 15: Dialogs & Messages (5 methods)

- `showMessage(message)` -- overlay with OK button, `CustomInformation` state
- `showErrorMessage(message, isCritical)` -- overlay. `isCritical` disables Ignore button.
- `showMessageBox(title, markdownMessage, type)` -- `MessageManager::callAsync` -> `PresetHandler::showMessageWindow`. Type maps to `PresetHandler::IconType`.
- `showYesNoWindow(title, markdownMessage, callback)` -- async yes/no dialog with `WeakCallbackHolder`
- `openWebsite(url)` -- validates URL, launches in browser after 300ms delay

### Group 16: Clipboard (2 methods)

- `copyToClipboard(text)` / `getClipboardContent()` -- wraps `SystemClipboard`

### Group 17: Transport & Host (2 methods)

- `getPlayHead()` -- returns `DynamicObject*` from `MainController::hostInfo`. **NOTE:** The property-setting code in `MainController::setHostBpm()` is entirely commented out, so this object may be empty/stale. The `TransportHandler` (created via `createTransportHandler()`) is the modern replacement for host transport information.
- `isControllerUsedByAutomation(ccNumber)` -- accepts single int or `[channel, ccNumber]` array. Returns automation index or -1.

### Group 18: Undo System (4 methods)

- `undo()` / `redo()` -- checks for `%SCRIPT_TRANSACTION%` marker. If found, executes synchronously. Otherwise dispatches to message thread via `MessageManager::callAsync`.
- `clearUndoHistory()` -- errors if called during undo/redo
- `performUndoAction(thisObject, undoAction)` -- creates `ScriptUndoableAction` (inner struct that wraps `WeakCallbackHolder`). The action callback receives a single boolean arg: `false` on perform, `true` on undo. Thread-aware: calls synchronously on scripting thread, async on message thread.

### Group 19: Audio Rendering (2 methods)

- `renderAudio(eventList, finishCallback)` -- creates `AudioRenderer` thread (extends `AudioRendererBase`). Renders MIDI events to audio buffers. Callback receives `{channels, finished, progress}`.
- `playBuffer(bufferData, callback, fileSampleRate)` -- previews audio buffer. Lazily creates `PreviewHandler`. Callback gets `(isPlaying, position)` args. If `fileSampleRate <= 0`, uses `getSampleRate()`.

### Group 20: Miscellaneous (5 methods)

- `getPlayHead()` -- (see Group 17)
- `getFilterModeList()` -- returns `FilterModeObject` (special scripting object with filter mode constants)
- `getSettingsWindowObject()` -- **DEPRECATED**: reports error, returns undefined.
- `isMpeEnabled()` -- checks `MidiControlAutomationHandler::getMPEData().isMpeEnabled()`
- `quit()` -- calls `JUCEApplication::quit()` only if `IS_STANDALONE_APP`
- `extendTimeOut(ms)` -- extends compilation timeout, no-op in compiled plugins
- `getSystemTime(bool)` -- ISO 8601 timestamp (also available via Date class)
- `getExtraDefinitionsInBackend()` -- returns extra definitions JSON, backend only
- `reloadAllSamples()` -- iterates all ModulatorSamplers, calls `reloadSampleMap()` via `killVoicesAndCall()`
- `getPreloadProgress()` / `getPreloadMessage()` / `setPreloadMessage()` -- sample preload status
- `logSettingWarning(methodName)` -- internal, not exposed? Actually in base JSON as method #148... wait, let me check.

---

## Deprecated / Migrated Methods

Several Engine methods are deprecated in favor of the `Settings` class (a sibling class in `ScriptingApi`):

| Engine Method | Settings Equivalent | Deprecation Mechanism |
|--------------|--------------------|-----------------------|
| `getZoomLevel()` | `Settings.getZoomLevel()` | `logSettingWarning()` console message |
| `setZoomLevel()` | `Settings.setZoomLevel()` | `logSettingWarning()` console message |
| `setDiskMode()` | `Settings.setDiskMode()` | `logSettingWarning()` console message |

Additionally:
- `loadFont()` is deprecated in favor of `loadFontAs()` -- emits `debugError()` warning
- `getSettingsWindowObject()` -- hard deprecated, `reportScriptError("Deprecated")`

The `logSettingWarning` method is an internal helper that emits: `"Engine.{method}() is deprecated. Use Settings.{method}() instead."`

---

## Preprocessor Guards Summary

| Guard | Affected Methods |
|-------|-----------------|
| `USE_BACKEND` | `loadFontAs`, `loadImageIntoPool`, `clearSampleMapPool`, `clearMidiFilePool`, `rebuildCachedPools`, `getSampleFilesFromDirectory`, `getExtraDefinitionsInBackend`, `loadAudioFilesIntoPool` (partial), `reloadAllSamples` (different path) |
| `USE_BACKEND` or `USE_COPY_PROTECTION` | `getProjectInfo` (licencee field) |
| `IS_STANDALONE_APP` | `isPlugin`, `quit` |
| `HISE_INCLUDE_BX_LICENSER` | `createBXLicenser` |
| `HISE_INCLUDE_NKS_SDK` | `createNKSManager` |
| `HISE_INCLUDE_LORIS` | `getLorisManager` |
| `READ_ONLY_FACTORY_PRESETS` | `isUserPresetReadOnly` (frontend path) |
| `HISE_USE_EXTENDED_TEMPO_VALUES` | Affects `TempoSyncer::Tempo` enum range |

---

## Threading Model

Engine is not directly audio-thread-safe. Key threading observations:

1. **Most methods run on the scripting thread** during `onInit` or control callbacks.
2. **`reloadAllSamples()`** explicitly uses `killVoicesAndCall()` to safely execute on the sample loading thread.
3. **`showYesNoWindow()`** and **`showMessageBox()`** dispatch to the message thread via `MessageManager::callAsync`.
4. **`undo()`/`redo()`** check if current operation is a script transaction (synchronous) or general undo (async message thread).
5. **`performUndoAction()`** creates a `ScriptUndoableAction` that is thread-aware: calls synchronously on scripting/loading threads, async on message thread.
6. **`renderAudio()`** spawns a background `Thread` for audio rendering.
7. **`playBuffer()`** uses a `PooledUIUpdater::SimpleTimer` for progress callbacks.
8. **`getUptime()`** has special audio-callback-aware behavior: if called from a MIDI callback with an active `HiseEvent`, it adds the event timestamp offset for sub-sample accuracy.

---

## Key Infrastructure Patterns

### MainController Access Pattern
Nearly every method accesses the `MainController` via:
```cpp
getProcessor()->getMainController()
// or
getScriptProcessor()->getMainController_()
```
These two paths provide equivalent access. The `ScriptingObject` base class provides both.

### Backend vs Frontend Branching
Many methods have fundamentally different implementations:
- **Backend**: reads from `HiseSettings` via `GET_HISE_SETTING()` macro, accesses `ProjectHandler` directly
- **Frontend**: reads from `FrontendHandler` static methods, uses embedded/baked data

### DynamicObject Return Pattern
Methods returning structured data construct `DynamicObject` instances on the heap:
```cpp
auto obj = new DynamicObject();
obj->setProperty("key", value);
return obj;  // implicit var wrapping via ReferenceCountedObject
```
Used by: `getProjectInfo()`, `getSystemStats()`, `getPlayHead()`, `getSettingsWindowObject()`

### Pool/Resource Loading Pattern
Pool operations follow: get pool reference -> load from reference -> cache strongly:
```cpp
PoolReference ref(mc, path, FileHandlerBase::AudioFiles);
pool->loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
```

### Expansion-Aware Resource Resolution
Several pool methods check for active expansion:
```cpp
if (auto e = mc->getExpansionHandler().getExpansionForWildcardReference(ref))
    pool = e->pool.get();
```

---

## Inner Classes

### PreviewHandler (lines 2258-2393)
Manages audio buffer preview playback. Extends `ControlledObject`, `AsyncUpdater`, `BufferPreviewListener`.

- Creates lazily on first `playBuffer()` call
- Registers as `BufferPreviewListener` on MainController
- Contains inner `Job` class that wraps buffer data and callback
- `Job` uses `PooledUIUpdater::SimpleTimer` for position update callbacks
- Handles mono-to-stereo expansion (duplicates channel 0 to channel 1)
- `previewStateChanged()` triggers async update to notify script when playback ends

### ScriptUndoableAction (lines 2083-2157)
A `UndoableAction` subclass for scriptable undo/redo. Extends `ControlledObject`.

- Wraps a `WeakCallbackHolder` with `setHighPriority()` for synchronous execution when possible
- `perform()` calls callback with `false`, `undo()` calls with `true`
- Thread-aware dispatch: synchronous on scripting/loading threads, async on message thread

### AudioRenderer (lines 2191-2251)
Extends `AudioRendererBase` (a Thread). Converts MIDI event lists to audio.

- Constructor takes event list (array of `ScriptingMessageHolder`), builds `HiseEventBuffer` list
- Splits events into multiple buffers at `HISE_EVENT_BUFFER_SIZE` boundaries
- `callUpdateCallback()` returns `{channels, finished, progress}` object
- Temporarily removes audio thread ID during callback to allow script execution

---

## ValueToTextConverter Modes

Used by `getTextForValue()` and `getValueForText()`. Available modes:

| Mode String | Forward (double -> String) | Inverse (String -> double) |
|-------------|---------------------------|---------------------------|
| `"Frequency"` | `"30 Hz"`, `"1.5 kHz"` | Parses `k` suffix |
| `"Time"` | `"500ms"`, `"1.5s"` | Parses `s`/`ms` suffix |
| `"TempoSync"` | `"1/4"`, `"1/8T"` | TempoSyncer index lookup |
| `"Pan"` | `"C"`, `"50L"`, `"50R"` | Parses L/R suffix |
| `"NormalizedPercentage"` | `"75%"` | Divides by 100 |
| `"Decibel"` | `"-6.0 dB"`, `"-INF"` | Handles -INF |
| `"Semitones"` | `"+2 st"`, `"-0.50 st"` | Direct parse |

Source: `ValueToTextConverter` struct in `hi_tools/hi_tools/MiscToolClasses.h` line 3044.

---

## TempoSyncer Enum Values

The `getTempoName()` and `getMilliSecondsForTempo()` methods use the `TempoSyncer::Tempo` enum:

**Standard values (default, no `HISE_USE_EXTENDED_TEMPO_VALUES`):**

| Index | Enum Name | Display Name |
|-------|-----------|-------------|
| 0 | `Whole` | 1/1 |
| 1 | `HalfDuet` | 1/2D |
| 2 | `Half` | 1/2 |
| 3 | `HalfTriplet` | 1/2T |
| 4 | `QuarterDuet` | 1/4D |
| 5 | `Quarter` | 1/4 |
| 6 | `QuarterTriplet` | 1/4T |
| 7 | `EighthDuet` | 1/8D |
| 8 | `Eighth` | 1/8 |
| 9 | `EighthTriplet` | 1/8T |
| 10 | `SixteenthDuet` | 1/16D |
| 11 | `Sixteenth` | 1/16 |
| 12 | `SixteenthTriplet` | 1/16T |
| 13 | `ThirtyTwoDuet` | 1/32D |
| 14 | `ThirtyTwo` | 1/32 |
| 15 | `ThirtyTwoTriplet` | 1/32T |
| 16 | `SixtyForthDuet` | 1/64D |
| 17 | `SixtyForth` | 1/64 |
| 18 | `SixtyForthTriplet` | 1/64T |

**Extended values (with `HISE_USE_EXTENDED_TEMPO_VALUES`):**
Prepends: `EightBar` (0), `SixBar` (1), `FourBar` (2), `ThreeBar` (3), `TwoBars` (4), then `Whole` (5), and all others shifted by 5.

Source: `TempoSyncer` class in `hi_tools/hi_tools/MiscToolClasses.h` line 2169.

---

## getPlayHead() -- Host Info Object

`getPlayHead()` returns `MainController::hostInfo`, a `DynamicObject::Ptr`.

**IMPORTANT:** The code that populates this object in `MainController::setHostBpm()` (line 1690-1720) is **entirely commented out**. The `hostInfo` object is created as an empty `DynamicObject` in the constructor (line 194) but never populated with property values.

This means `getPlayHead()` returns an empty or minimally-populated object. The intended properties were:
- `bpm`, `timeSigNumerator`, `timeSigDenominator`
- `timeInSamples`, `timeInSeconds`, `editOriginTime`
- `ppqPosition`, `ppqPositionOfLastBarStart`
- `frameRate`, `isPlaying`, `isRecording`
- `ppqLoopStart`, `ppqLoopEnd`, `isLooping`

The modern replacement for transport information is `TransportHandler` (created via `createTransportHandler()`), which provides callback-based transport events via `TempoListener` and `PooledUIUpdater::Listener`.

---

## getProjectInfo() -- Return Object Schema

Returns a DynamicObject with these properties:

| Property | Backend Source | Frontend Source |
|----------|--------------|----------------|
| `Company` | `HiseSettings::User::Company` | `FrontendHandler::getCompanyName()` |
| `CompanyURL` | `HiseSettings::User::CompanyURL` | `FrontendHandler::getCompanyWebsiteName()` |
| `CompanyCopyright` | `HiseSettings::User::CompanyCopyright` | `FrontendHandler::getCompanyCopyright()` |
| `ProjectName` | `HiseSettings::Project::Name` | `FrontendHandler::getProjectName()` |
| `ProjectVersion` | `HiseSettings::Project::Version` | `FrontendHandler::getVersionString()` |
| `EncryptionKey` | `HiseSettings::Project::EncryptionKey` | `FrontendHandler::getExpansionKey()` |
| `HISEBuild` | `GlobalSettingManager::getHiseVersion()` | same |
| `BuildDate` | `Time::getCompilationDate()` | same |
| `LicensedEmail` | from `getLicenseUnlocker()` | same (if `USE_COPY_PROTECTION`) |

---

## getSystemStats() -- Return Object Schema

| Property | Source |
|----------|--------|
| `OperatingSystemName` | `SystemStats::getOperatingSystemName()` |
| `OperatingSystem64Bit` | `SystemStats::isOperatingSystem64Bit()` |
| `LogonName` | `SystemStats::getLogonName()` |
| `FullUserName` | `SystemStats::getFullUserName()` |
| `ComputerName` | `SystemStats::getComputerName()` |
| `UserLanguage` | `SystemStats::getUserLanguage()` |
| `UserRegion` | `SystemStats::getUserRegion()` |
| `DisplayLanguage` | `SystemStats::getDisplayLanguage()` |
| `NumCpus` | `SystemStats::getNumCpus()` |
| `NumPhysicalCpus` | `SystemStats::getNumPhysicalCpus()` |
| `CpuSpeedInMegahertz` | `SystemStats::getCpuSpeedInMegahertz()` |
| `CpuVendor` | `SystemStats::getCpuVendor()` |
| `CpuModel` | `SystemStats::getCpuModel()` |
| `MemorySizeInMegabytes` | `SystemStats::getMemorySizeInMegabytes()` |
| `isDarkMode` | `Desktop::getInstance().isDarkModeActive()` |

---

## logSettingWarning -- Deprecation Migration

The `logSettingWarning` method (line 3722) is used to emit deprecation warnings for methods that have been migrated to the `Settings` class. It constructs:

```
"Engine.{methodName}() is deprecated. Use Settings.{methodName}() instead."
```

This is called by `getZoomLevel()`, `setZoomLevel()`, and `setDiskMode()` at the start of their implementation, but the Engine method still works (it just warns).

Note: `logSettingWarning` appears in the base JSON as a public method, but it is an internal helper -- it is registered like any other API method and technically callable from script, though it serves no useful purpose for users.
