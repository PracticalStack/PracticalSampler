# Engine -- Class Analysis

## Brief
Central factory, utility, and system-query namespace for object creation, unit conversion, and global state.

## Purpose
Engine is the primary namespace-style API class in HISE, providing the global `Engine` object accessible from every script processor. It serves as the central factory for creating most scripting API objects (Broadcasters, Timers, MidiLists, DspNetworks, ExpansionHandlers, etc.), offers extensive unit conversion utilities (dB/gain, MIDI/frequency, samples/milliseconds, tempo sync), manages global engine state (BPM, pitch, user presets, expansions), and provides system information queries and resource pool operations. With 147 methods, it is the largest API class in HISE and acts as the primary entry point for most scripting workflows.

## Details

### Method Organization

Engine's 147 methods group into these functional areas:

#### Object Factories (27 methods)
The largest group. Each `create*` method instantiates a scripting API object and typically takes the current script processor as the owner. Most are simple constructors, but notable exceptions include:
- `createGlobalScriptLookAndFeel()` -- singleton pattern; returns an existing global LAF if one is already registered, otherwise creates a new one
- `createDspNetwork()` -- requires the processor to be a `scriptnode::DspNetwork::Holder`; reports an error if not
- `createBXLicenser()` and `createNKSManager()` -- guarded by `HISE_INCLUDE_BX_LICENSER` and `HISE_INCLUDE_NKS_SDK` preprocessor flags; report errors if the SDK is not enabled
- `createAndRegister*` methods (AudioFile, RingBuffer, SliderPackData, TableData) -- take an `int index` for slot-based registration in the `ExternalDataHolder` system

#### Reference Accessors (4 methods)
`getDspNetworkReference`, `getGlobalRoutingManager`, `getLorisManager`, and `getComplexDataReference` look up existing objects rather than creating new ones. `getComplexDataReference` accepts a `dataType` string (`"Table"`, `"SliderPack"`, `"AudioFile"`, `"FilterCoefficients"`, `"DisplayBuffer"`) to retrieve complex data from any `ExternalDataHolder` module.

#### Unit Conversion -- Audio/Time (16 methods)
Pure math/conversion with no side effects. Includes sample-rate-based conversions (samples <-> milliseconds), tempo-based conversions using the current host BPM, and explicit-tempo variants (`*WithTempo` suffixes). Tempo sync methods delegate to the `TempoSyncer` utility class.

#### Unit Conversion -- Musical (6 methods)
dB <-> gain factor, MIDI note number <-> frequency, semitones <-> pitch ratio, MIDI note number <-> note name string.

#### Audio Engine Control (8 methods)
Query and configure the audio engine: `getSampleRate`, `getBufferSize`, `getNumPluginChannels`, `setMinimumSampleRate` (oversampling floor), `setMaximumBlockSize` (buffer splitting), `getControlRateDownsamplingFactor` (compile-time `HISE_EVENT_RASTER`, default 8), `setLatencySamples`/`getLatencySamples`.

#### Global State (9 methods)
Read/write global engine state: host BPM (`getHostBpm`/`setHostBpm`, -1 = sync to host), uptime, CPU/memory usage, active voice count, global pitch factor (clamped to -12..12 semitones), and `allNotesOff`.

#### User Preset Management (11 methods)
Load/save/navigate user presets, query the preset list, manage read-only status, set tag lists for the preset browser, configure macro system with `setFrontendMacros`, and add module state with `addModuleStateToUserPreset`.

#### Expansion Management (3 methods)
`getExpansionList`, `setCurrentExpansion`, and `getWavetableList`.

#### Pool and Resource Management (8 methods)
Load audio files and images into pools, clear MIDI/samplemap pools, rebuild cached pools, get sample file references, and control duplicate sample detection. Most pool-write operations are backend-only.

#### Font Management (3 methods)
`loadFont` (deprecated), `loadFontAs` (platform-agnostic replacement), `setGlobalFont`.

#### JSON and Data Serialization (5 methods)
Dump/load JSON to/from files in the UserPresets directory, compress/uncompress JSON via zstd+Base64, and decode Base64 ValueTrees.

#### String and Formatting (8 methods)
Number formatting (`doubleToString`, `intToHexString`), string width measurement, regex matching, value-to-text conversion with modes (`"Frequency"`, `"Time"`, `"TempoSync"`, `"Pan"`, `"NormalizedPercentage"`, `"Decibel"`, `"Semitones"`), and array sorting with a custom comparator.

#### System Information (8 methods)
OS detection (`getOS` returns `"WIN"`, `"LINUX"`, or `"OSX"`), detailed system stats, device type/resolution, plugin vs standalone detection, HISE vs frontend detection, project version/name, and `getProjectInfo` (returns a JSON object with Company, ProjectName, ProjectVersion, EncryptionKey, HISEBuild, BuildDate, etc.).

#### UI and Display (6 methods)
Keyboard coloring (`setKeyColour`, `setLowestKeyToDisplay`), zoom level (deprecated), disk mode (deprecated), and master peak level.

#### Dialogs and Messages (5 methods)
Show overlay messages, error messages, message boxes with icons, yes/no dialogs with async callbacks, and open URLs in browser.

#### Clipboard (2 methods)
`copyToClipboard` and `getClipboardContent`.

#### Transport and Host (2 methods)
`getPlayHead` returns host transport info (note: the property-population code in `MainController::setHostBpm` is currently commented out, so this may return an empty object -- use `createTransportHandler()` instead). `isControllerUsedByAutomation` checks if a CC number is used by parameter automation.

#### Undo System (4 methods)
`undo`/`redo` (thread-aware: synchronous for script transactions, async for message thread), `clearUndoHistory`, and `performUndoAction` (wraps a callback as an `UndoableAction` -- callback receives `false` on perform, `true` on undo).

#### Audio Rendering (2 methods)
`renderAudio` renders MIDI events to audio buffers on a background thread. `playBuffer` previews audio buffers with a progress callback.

### Backend vs Frontend Differences

Many Engine methods behave differently depending on the build target:
- **Pool operations** (`loadImageIntoPool`, `clearSampleMapPool`, `clearMidiFilePool`, `rebuildCachedPools`, `getSampleFilesFromDirectory`) -- backend only; no-op or unavailable in compiled plugins
- **Font loading** (`loadFontAs`) -- actively loads from the Images folder in backend; no-op in frontend (fonts are baked at compile time)
- **Project info** (`getProjectInfo`, `getName`, `getVersion`) -- reads from `HiseSettings` in backend, from `FrontendHandler` in frontend
- **Extra definitions** (`getExtraDefinitionsInBackend`) -- only available in backend

### Deprecation Pattern

Several methods are migrated to the `Settings` class:

| Deprecated Method | Replacement | Mechanism |
|---|---|---|
| `getZoomLevel()` | `Settings.getZoomLevel()` | Console warning via `logSettingWarning()` |
| `setZoomLevel()` | `Settings.setZoomLevel()` | Console warning via `logSettingWarning()` |
| `setDiskMode()` | `Settings.setDiskMode()` | Console warning via `logSettingWarning()` |

Additionally: `loadFont()` is deprecated in favor of `loadFontAs()` (emits `debugError`), and `getSettingsWindowObject()` is hard-deprecated (reports error, returns undefined).

### ValueToTextConverter Modes

Used by `getTextForValue()` and `getValueForText()`:

| Mode | Example Output | Parsing Behavior |
|---|---|---|
| `"Frequency"` | `"1.5 kHz"` | Recognizes `k` suffix |
| `"Time"` | `"500ms"`, `"1.5s"` | Recognizes `s`/`ms` suffixes |
| `"TempoSync"` | `"1/4"`, `"1/8T"` | TempoSyncer index lookup |
| `"Pan"` | `"C"`, `"50L"`, `"50R"` | Recognizes L/R suffix |
| `"NormalizedPercentage"` | `"75%"` | Divides by 100 |
| `"Decibel"` | `"-6.0 dB"`, `"-INF"` | Handles -INF |
| `"Semitones"` | `"+2 st"` | Direct parse |

### TempoSyncer Index Values

Used by `getMilliSecondsForTempo()` and `getTempoName()`:

| Index | Name |
|---|---|
| 0 | 1/1 |
| 1 | 1/2D |
| 2 | 1/2 |
| 3 | 1/2T |
| 4 | 1/4D |
| 5 | 1/4 |
| 6 | 1/4T |
| 7 | 1/8D |
| 8 | 1/8 |
| 9 | 1/8T |
| 10 | 1/16D |
| 11 | 1/16 |
| 12 | 1/16T |
| 13 | 1/32D |
| 14 | 1/32 |
| 15 | 1/32T |
| 16 | 1/64D |
| 17 | 1/64 |
| 18 | 1/64T |

With `HISE_USE_EXTENDED_TEMPO_VALUES` enabled, indices 0-4 become EightBar, SixBar, FourBar, ThreeBar, TwoBars, and all standard values shift by 5.

### getProjectInfo() Return Schema

| Property | Backend Source | Frontend Source |
|---|---|---|
| `Company` | `HiseSettings::User::Company` | `FrontendHandler::getCompanyName()` |
| `CompanyURL` | `HiseSettings::User::CompanyURL` | `FrontendHandler::getCompanyWebsiteName()` |
| `CompanyCopyright` | `HiseSettings::User::CompanyCopyright` | `FrontendHandler::getCompanyCopyright()` |
| `ProjectName` | `HiseSettings::Project::Name` | `FrontendHandler::getProjectName()` |
| `ProjectVersion` | `HiseSettings::Project::Version` | `FrontendHandler::getVersionString()` |
| `EncryptionKey` | `HiseSettings::Project::EncryptionKey` | `FrontendHandler::getExpansionKey()` |
| `HISEBuild` | `GlobalSettingManager::getHiseVersion()` | same |
| `BuildDate` | `Time::getCompilationDate()` | same |
| `LicensedEmail` | from `getLicenseUnlocker()` | same (if `USE_COPY_PROTECTION`) |

### getSystemStats() Return Schema

| Property | Source |
|---|---|
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

## obtainedVia
Implicit global `Engine` object -- available in every script processor without creation.

## minimalObjectToken


## Constants
None. Engine registers zero constants via `addConstant()`. All constant-like data is returned via methods (`getOS()`, `getFilterModeList()`, etc.).

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|---|---|---|
| `Engine.loadUserPreset("MyPreset")` in `onInit` | Call `loadUserPreset` from a button callback or timer | Loading user presets during initialization is explicitly rejected with an error message. |
| `Engine.getPlayHead().bpm` | `Engine.getHostBpm()` or use `createTransportHandler()` | The `getPlayHead()` return object's property-population code is commented out; use dedicated BPM methods or TransportHandler instead. |
| `Engine.setZoomLevel(1.5)` | `Settings.setZoomLevel(1.5)` | `setZoomLevel` is deprecated; use the Settings class equivalent. |
| `Engine.getMacroName(0)` | `Engine.getMacroName(1)` | Macro indices are 1-based (1-8), not 0-based. |

## codeExample
```javascript
// Engine is an implicit global -- no creation needed.
// Unit conversion
var freq = Engine.getFrequencyForMidiNoteNumber(69); // 440.0
var db = Engine.getDecibelsForGainFactor(0.5);        // ~-6.0

// System queries
var sr = Engine.getSampleRate();
var os = Engine.getOS(); // "WIN", "OSX", or "LINUX"
```

## Alternatives
- `Synth` -- Controls the parent synthesiser's voice management, MIDI processing, and module tree traversal (Engine handles global utilities and object factories).
- `Settings` -- Manages audio device configuration and standalone app preferences (replaces deprecated Engine zoom/disk methods).

## Related Preprocessors
`USE_BACKEND`, `IS_STANDALONE_APP`, `HISE_INCLUDE_BX_LICENSER`, `HISE_INCLUDE_NKS_SDK`, `HISE_INCLUDE_LORIS`, `READ_ONLY_FACTORY_PRESETS`, `HISE_USE_EXTENDED_TEMPO_VALUES`, `HISE_NUM_MACROS`, `USE_COPY_PROTECTION`

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- Engine.getTextForValue / Engine.getValueForText -- value-check (logged)
