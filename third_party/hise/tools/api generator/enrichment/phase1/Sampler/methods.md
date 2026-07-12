# Sampler -- Method Analysis

## clearSampleMap

**Signature:** `Integer clearSampleMap()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Kills all voices via killAllVoicesAndCall, lambda allocation.
**Minimal Example:** `var ok = {obj}.clearSampleMap();`

**Description:**
Clears the current sample map by removing all samples. Kills all active voices before clearing. Returns true on success.

**Parameters:**

(No parameters.)

**Pitfalls:**
- This is an asynchronous operation internally -- the sample map clear happens via `killAllVoicesAndCall`. The return value indicates the clear was scheduled, not necessarily completed.

**Cross References:**
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.loadSampleMapFromJSON$`

---

## createListFromGUISelection

**Signature:** `Array createListFromGUISelection()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** USE_BACKEND only. Acquires MessageManagerLock for GUI thread safety.
**Minimal Example:** `var selection = {obj}.createListFromGUISelection();`

**Description:**
Returns an array of Sample objects corresponding to the samples currently selected in the HISE sample editor GUI. Only works in the HISE IDE (USE_BACKEND). Returns an empty array in exported plugins.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns an empty array silently in exported plugins (compiled with USE_FRONTEND). No error is reported.

**Cross References:**
- `$API.Sampler.createListFromScriptSelection$`
- `$API.Sampler.createSelection$`
- `$API.Sampler.setGUISelection$`

**Related Preprocessors:**
USE_BACKEND

---

## createListFromScriptSelection

**Signature:** `Array createListFromScriptSelection()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates ScriptingSamplerSound objects on the heap.
**Minimal Example:** `var list = {obj}.createListFromScriptSelection();`

**Description:**
Converts the legacy script selection (populated by `selectSounds()`) into an array of Sample objects. This bridges the legacy selection API to the modern Sample-based API.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.selectSounds$`
- `$API.Sampler.createSelection$`

---

## createSelection

**Signature:** `Array createSelection(String regex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Allocates ScriptingSamplerSound objects.
**Minimal Example:** `var sel = {obj}.createSelection(".*kick.*");`

**Description:**
Creates an array of Sample objects matching the given regex pattern against sample file names. This is the modern replacement for the legacy `selectSounds()` workflow.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| regex | String | no | Regular expression to match against sample file names | Valid regex pattern |

**Cross References:**
- `$API.Sampler.createSelectionFromIndexes$`
- `$API.Sampler.createSelectionWithFilter$`
- `$API.Sampler.selectSounds$`

---

## createSelectionFromIndexes

**Signature:** `Array createSelectionFromIndexes(Array indexData)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Allocates ScriptingSamplerSound objects.
**Minimal Example:** `var sel = {obj}.createSelectionFromIndexes([0, 1, 2]);`

**Description:**
Creates an array of Sample objects from sample indices. Accepts an array of integer indices, a single integer index, or -1 to select all samples.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| indexData | Array | yes | Array of sample indices, a single index, or -1 for all | -1 or valid sample indices |

**Cross References:**
- `$API.Sampler.createSelection$`
- `$API.Sampler.createSelectionWithFilter$`

---

## createSelectionWithFilter

**Signature:** `Array createSelectionWithFilter(Function filterFunction)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Iterates all samples and calls the filter function for each. Allocates.
**Minimal Example:** `var sel = {obj}.createSelectionWithFilter(myFilter);`

**Description:**
Creates an array of Sample objects by evaluating a filter function for each sample. The function is called with `this` set to a Sample object (no arguments). Return a non-zero value to include the sample.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| filterFunction | Function | no | Filter function. `this` is a Sample object, return non-zero to include | Must return Integer |

**Callback Signature:** filterFunction(this: Sample)

**Example:**
```javascript:velocity-filter
// Filter samples by velocity range
inline function velocityFilter()
{
    return this.get(Sampler.HiVel) > 64;
};

const var loudSamples = Sampler.createSelectionWithFilter(velocityFilter);
Console.print("Found " + loudSamples.length + " loud samples");
```

```json:testMetadata:velocity-filter
{
  "testable": false,
  "skipReason": "Requires loaded sample map with velocity-mapped samples"
}
```

**Cross References:**
- `$API.Sampler.createSelection$`
- `$API.Sampler.createSelectionFromIndexes$`

---

## enableRoundRobin

**Signature:** `undefined enableRoundRobin(Integer shouldUseRoundRobin)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `{obj}.enableRoundRobin(false);`

**Description:**
Enables or disables automatic round robin group cycling. Must be disabled before using manual group selection methods like `setActiveGroup()`, `setMultiGroupIndex()`, or `getRRGroupsForMessage()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseRoundRobin | Integer | no | true to enable automatic RR, false to disable | -- |

**Cross References:**
- `$API.Sampler.setActiveGroup$`
- `$API.Sampler.setMultiGroupIndex$`
- `$API.Sampler.getRRGroupsForMessage$`

---

## getActiveRRGroup

**Signature:** `Integer getActiveRRGroup()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var group = {obj}.getActiveRRGroup();`

**Description:**
Returns the currently active round robin group index. Delegates to `getActiveRRGroupForEventId(-1)` for the global group state.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.getActiveRRGroupForEventId$`
- `$API.Sampler.setActiveGroup$`

---

## getActiveRRGroupForEventId

**Signature:** `Integer getActiveRRGroupForEventId(Integer eventId)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var group = {obj}.getActiveRRGroupForEventId(Message.getEventId());`

**Description:**
Returns the active round robin group for a specific event. Pass -1 for the global group state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | The event ID, or -1 for global state | -1 or valid event ID |

**Cross References:**
- `$API.Sampler.getActiveRRGroup$`
- `$API.Sampler.setActiveGroupForEventId$`

---

## getAttribute

**Signature:** `Number getAttribute(Number index)`
**Return Type:** `Number`
**Call Scope:** safe
**Minimal Example:** `var val = {obj}.getAttribute({obj}.PreloadSize);`

**Description:**
Returns the value of a sampler module attribute (processor parameter) by index. Uses the ModulatorSampler parameter indices which include inherited ModulatorSynth parameters.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Number | yes | Parameter index | Valid parameter index |

**Cross References:**
- `$API.Sampler.setAttribute$`
- `$API.Sampler.getAttributeId$`
- `$API.Sampler.getAttributeIndex$`
- `$API.Sampler.getNumAttributes$`

---

## getAttributeId

**Signature:** `String getAttributeId(Number parameterIndex)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getAttributeId(0);`

**Description:**
Returns the string identifier of a sampler parameter by its index.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Number | yes | Parameter index | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.Sampler.getAttributeIndex$`
- `$API.Sampler.getAttribute$`

---

## getAttributeIndex

**Signature:** `Integer getAttributeIndex(String parameterId)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var idx = {obj}.getAttributeIndex("PreloadSize");`

**Description:**
Returns the parameter index for a given parameter identifier string. Returns -1 if not found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterId | String | yes | The parameter identifier string | Valid parameter name |

**Cross References:**
- `$API.Sampler.getAttributeId$`
- `$API.Sampler.getAttribute$`

---

## getAudioWaveformContentAsBase64

**Signature:** `String getAudioWaveformContentAsBase64(JSON presetObj)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Parses audio file, allocates, compresses to base64.
**Minimal Example:** `var b64 = {obj}.getAudioWaveformContentAsBase64(presetData);`

**Description:**
Converts an AudioWaveform user preset object into a base64-encoded sample map. Reads `data` (file path), `rangeStart`, and `rangeEnd` properties from the preset object, parses the audio file metadata, then compresses the resulting sample map to base64.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| presetObj | JSON | no | Preset object with `data`, `rangeStart`, `rangeEnd` properties | Must contain valid file path in `data` |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| data | String | Absolute file path to the audio file |
| rangeStart | Integer | Sample start offset (0 if not set) |
| rangeEnd | Integer | Sample end offset (0 if not set) |

**Cross References:**
- `$API.Sampler.parseSampleFile$`
- `$API.Sampler.loadSampleMapFromBase64$`

---

## getComplexGroupManager

**Signature:** `ScriptObject getComplexGroupManager()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ScriptingComplexGroupManager object.
**Minimal Example:** `var cgm = {obj}.getComplexGroupManager();`

**Description:**
Returns a ComplexGroupManager object for advanced group management on this sampler. The ComplexGroupManager provides more sophisticated round robin and group handling beyond the basic `setActiveGroup`/`setMultiGroupIndex` methods.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.setActiveGroup$`
- `$API.Sampler.setMultiGroupIndex$`
- `$API.Sampler.enableRoundRobin$`

---

## getCurrentSampleMapId

**Signature:** `String getCurrentSampleMapId()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Returns String.
**Minimal Example:** `var id = {obj}.getCurrentSampleMapId();`

**Description:**
Returns the ID string of the currently loaded sample map. Returns an empty string if no sample map is loaded.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.getSampleMapList$`

---

## getMicPositionName

**Signature:** `String getMicPositionName(Integer channelIndex)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Returns String.
**Minimal Example:** `var name = {obj}.getMicPositionName(0);`

**Description:**
Returns the name (suffix) of a mic position channel by index. Only works with multi-mic samplers (more than one mic position or static matrix enabled).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channelIndex | Integer | no | Zero-based channel index | 0 to getNumMicPositions()-1 |

**Pitfalls:**
- Reports a script error if the sampler has only one mic position and is not using static matrix. Check `getNumMicPositions()` first.

**Cross References:**
- `$API.Sampler.getNumMicPositions$`
- `$API.Sampler.purgeMicPosition$`
- `$API.Sampler.isMicPositionPurged$`

---

## getNumActiveGroups

**Signature:** `Integer getNumActiveGroups()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.getNumActiveGroups();`

**Description:**
Returns the number of currently active round robin groups.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.getActiveRRGroup$`
- `$API.Sampler.setActiveGroup$`

---

## getNumAttributes

**Signature:** `Integer getNumAttributes()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var num = {obj}.getNumAttributes();`

**Description:**
Returns the total number of processor parameters (attributes) available on this sampler module. This includes both the base ModulatorSynth parameters and sampler-specific parameters.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.getAttribute$`
- `$API.Sampler.setAttribute$`

---

## getNumMicPositions

**Signature:** `Integer getNumMicPositions()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var numMics = {obj}.getNumMicPositions();`

**Description:**
Returns the number of mic positions (channels) configured on this sampler.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.getMicPositionName$`
- `$API.Sampler.isMicPositionPurged$`
- `$API.Sampler.purgeMicPosition$`

---

## getNumSelectedSounds

**Signature:** `Integer getNumSelectedSounds()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `var count = {obj}.getNumSelectedSounds();`

**Description:**
Returns the number of sounds in the legacy script selection (populated by `selectSounds()`). Part of the legacy selection API.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.selectSounds$`
- `$API.Sampler.getSoundProperty$`
- `$API.Sampler.createListFromScriptSelection$`

---

## getRRGroupsForMessage

**Signature:** `Integer getRRGroupsForMessage(Integer noteNumber, Integer velocity)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var groups = {obj}.getRRGroupsForMessage(60, 100);`

**Description:**
Returns the number of round robin groups that have samples mapped for the given note number and velocity combination. Requires round robin to be disabled first. Must call `refreshRRMap()` after loading a sample map and before using this method.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| noteNumber | Integer | no | MIDI note number | 0-127 |
| velocity | Integer | no | MIDI velocity | 0-127 |

**Pitfalls:**
- Requires `enableRoundRobin(false)` and `refreshRRMap()` to have been called first. Reports a script error if round robin is still enabled.

**Cross References:**
- `$API.Sampler.enableRoundRobin$`
- `$API.Sampler.refreshRRMap$`

---

## getReleaseStartOptions

**Signature:** `JSON getReleaseStartOptions()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Allocates JSON object.
**Minimal Example:** `var opts = {obj}.getReleaseStartOptions();`

**Description:**
Returns the current release start options as a JSON object. Requires the HISE_SAMPLER_ALLOW_RELEASE_START preprocessor to be enabled (enabled by default).

**Parameters:**

(No parameters.)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| ReleaseFadeTime | Integer | Fade time in samples (0-44100, default 4096) |
| FadeGamma | Double | Gamma curve for fade (0.125-4.0, default 1.0) |
| UseAscendingZeroCrossing | Integer | Whether to use ascending zero crossing (default false) |
| GainMatchingMode | String | "None", "Volume", or "Offset" |
| PeakSmoothing | Double | Peak smoothing factor (default 0.96) |

**Cross References:**
- `$API.Sampler.setReleaseStartOptions$`
- `$API.Sampler.setAllowReleaseStart$`

**Related Preprocessors:**
HISE_SAMPLER_ALLOW_RELEASE_START

---

## getSampleMapAsBase64

**Signature:** `String getSampleMapAsBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Compresses ValueTree to zstd, base64 encodes. Allocates.
**Minimal Example:** `var b64 = {obj}.getSampleMapAsBase64();`

**Description:**
Returns the current sample map as a zstd-compressed, base64-encoded string. This can be stored and later restored with `loadSampleMapFromBase64()`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.loadSampleMapFromBase64$`
- `$API.Sampler.saveCurrentSampleMap$`

---

## getSampleMapList

**Signature:** `Array getSampleMapList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Allocates array and strings.
**Minimal Example:** `var maps = {obj}.getSampleMapList();`

**Description:**
Returns a sorted array of reference strings for all available sample maps in the pool.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.getCurrentSampleMapId$`

---

## getSoundProperty

**Signature:** `var getSoundProperty(Integer propertyIndex, Integer soundIndex)`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `var val = {obj}.getSoundProperty(Sampler.Root, 0);`

**Description:**
Returns the value of a sound property for a specific sound in the legacy script selection. The sound index refers to the selection populated by `selectSounds()`. Use the Sampler constants (e.g., `Sampler.Root`, `Sampler.LoKey`) for property indices.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyIndex | Integer | no | Sound property index (use Sampler constants) | 1-24 (SampleIds range) |
| soundIndex | Integer | no | Index into the legacy script selection | 0 to getNumSelectedSounds()-1 |

**Cross References:**
- `$API.Sampler.setSoundProperty$`
- `$API.Sampler.selectSounds$`
- `$API.Sampler.getNumSelectedSounds$`

---

## getTimestretchOptions

**Signature:** `JSON getTimestretchOptions()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Allocates JSON object.
**Minimal Example:** `var opts = {obj}.getTimestretchOptions();`

**Description:**
Returns the current timestretch configuration as a JSON object.

**Parameters:**

(No parameters.)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Mode | String | "Disabled", "VoiceStart", "TimeVariant", or "TempoSynced" |
| Tonality | Double | Tonality parameter (0.0-1.0) |
| SkipLatency | Integer | Whether to skip latency compensation (boolean) |
| NumQuarters | Double | Number of quarter notes for TempoSynced mode |
| PreferredEngine | String | Engine identifier string |

**Cross References:**
- `$API.Sampler.setTimestretchOptions$`
- `$API.Sampler.setTimestretchRatio$`

---

## importSamples

**Signature:** `Array importSamples(Array fileNameList, Integer skipExistingSamples)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Kills voices, acquires SampleLock, file I/O.
**Minimal Example:** `var newSamples = {obj}.importSamples(fileList, true);`

**Description:**
Imports audio files into the sampler and returns an array of Sample objects for the newly imported samples. Only available when HI_ENABLE_EXPANSION_EDITING is enabled (backend/expansion editing builds). Auto-assigns root notes starting from note 0. Extends script timeout for the duration of the import.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileNameList | Array | no | Array of file path strings or pool references | Non-empty array |
| skipExistingSamples | Integer | no | If true, skip files already loaded (by filename match) | -- |

**Pitfalls:**
- Returns an empty array silently in exported plugins (HI_ENABLE_EXPANSION_EDITING is disabled). No error is reported.
- Returns the input array unchanged if it is empty.

**Cross References:**
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.parseSampleFile$`

**Related Preprocessors:**
HI_ENABLE_EXPANSION_EDITING

---

## isMicPositionPurged

**Signature:** `Integer isMicPositionPurged(Integer micIndex)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var purged = {obj}.isMicPositionPurged(0);`

**Description:**
Returns whether a mic position channel is purged (disabled). Returns the inverse of the internal enabled state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| micIndex | Integer | no | Zero-based mic position index | 0 to getNumMicPositions()-1 |

**Pitfalls:**
- [BUG] Returns false silently for out-of-range indices instead of reporting an error.

**Cross References:**
- `$API.Sampler.purgeMicPosition$`
- `$API.Sampler.getNumMicPositions$`

---

## isNoteNumberMapped

**Signature:** `Integer isNoteNumberMapped(Integer noteNumber)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var mapped = {obj}.isNoteNumberMapped(60);`

**Description:**
Returns whether any samples are mapped to the given MIDI note number.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| noteNumber | Integer | no | MIDI note number to check | 0-127 |

**Cross References:**
- `$API.Sampler.getRRGroupsForMessage$`

---

## loadSampleForAnalysis

**Signature:** `var loadSampleForAnalysis(Integer soundIndex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Loads audio data into memory.
**Minimal Example:** `var buffers = {obj}.loadSampleForAnalysis(0);`

**Description:**
Loads a sample from the legacy script selection into a buffer array for analysis. The sound index refers to the selection populated by `selectSounds()`. Returns the audio data as an array of buffers (one per channel).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| soundIndex | Integer | no | Index into the legacy script selection | 0 to getNumSelectedSounds()-1 |

**Cross References:**
- `$API.Sampler.selectSounds$`
- `$API.Sampler.getNumSelectedSounds$`
- `$API.Sampler.createSelection$`

---

## loadSampleMap

**Signature:** `undefined loadSampleMap(String fileName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. killAllVoicesAndCall.
**Minimal Example:** `{obj}.loadSampleMap("{PROJECT_FOLDER}MySampleMap");`

**Description:**
Loads a sample map by its pool reference string. Kills all active voices before loading. The reference string should match the format returned by `getSampleMapList()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | Pool reference string for the sample map | Non-empty, valid pool reference |

**Pitfalls:**
- Reports a script error for empty strings or invalid pool references.

**Cross References:**
- `$API.Sampler.getSampleMapList$`
- `$API.Sampler.getCurrentSampleMapId$`
- `$API.Sampler.loadSampleMapFromJSON$`
- `$API.Sampler.loadSampleMapFromBase64$`
- `$API.Sampler.clearSampleMap$`

---

## loadSampleMapFromBase64

**Signature:** `undefined loadSampleMapFromBase64(String b64)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** killAllVoicesAndCall, decompression, allocation.
**Minimal Example:** `{obj}.loadSampleMapFromBase64(savedBase64);`

**Description:**
Loads a sample map from a zstd-compressed, base64-encoded string (as produced by `getSampleMapAsBase64()`). Kills all active voices before loading.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded, zstd-compressed sample map data | Valid encoded string |

**Cross References:**
- `$API.Sampler.getSampleMapAsBase64$`
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.loadSampleMapFromJSON$`

---

## loadSampleMapFromJSON

**Signature:** `undefined loadSampleMapFromJSON(Array jsonSampleList)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** killAllVoicesAndCall, ValueTree construction, allocation.
**Minimal Example:** `{obj}.loadSampleMapFromJSON(sampleArray);`

**Description:**
Loads a sample map from a JSON array of sample descriptor objects. Each object should contain properties matching the SampleIds constants (FileName, Root, LoKey, HiKey, etc.). Missing properties get defaults: LoVel=0, HiVel=127, LoKey=0, HiKey=127, Root=64, RRGroup=1. Kills all active voices before loading.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonSampleList | Array | no | Array of JSON objects with sample properties | Each must have at least FileName |

**Example:**
```javascript:load-json-samplemap
// Load a custom sample map from JSON
var samples = [
    {"FileName": "{PROJECT_FOLDER}Samples/kick.wav", "Root": 36, "LoKey": 36, "HiKey": 36},
    {"FileName": "{PROJECT_FOLDER}Samples/snare.wav", "Root": 38, "LoKey": 38, "HiKey": 38}
];

Sampler.loadSampleMapFromJSON(samples);
```

```json:testMetadata:load-json-samplemap
{
  "testable": false,
  "skipReason": "Requires audio files on disk"
}
```

**Cross References:**
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.loadSampleMapFromBase64$`
- `$API.Sampler.parseSampleFile$`

---

## loadSfzFile

**Signature:** `var loadSfzFile(var sfzFile)`
**Return Type:** `var`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. File I/O, parsing, killAllVoicesAndCall.
**Minimal Example:** `var error = {obj}.loadSfzFile(myFile);`

**Description:**
Loads a sample map from an SFZ file. Accepts a ScriptFile object or an absolute path string. Returns undefined on success, or an error string on failure (parsing error, no content, or unknown error). Extends script timeout for the parsing duration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sfzFile | ScriptObject | no | ScriptFile object or absolute path string | Must point to existing .sfz file |

**Pitfalls:**
- Uses an inverted return convention: undefined means success, a String means failure. Check with `isDefined(result)` to detect errors.

**Cross References:**
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.loadSampleMapFromJSON$`

---

## parseSampleFile

**Signature:** `JSON parseSampleFile(var sampleFile)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** File I/O, allocates DynamicObject.
**Minimal Example:** `var meta = {obj}.parseSampleFile(myFile);`

**Description:**
Parses an audio file and returns its metadata as a JSON object. The returned object contains properties compatible with `loadSampleMapFromJSON()` (Root, SampleStart, SampleEnd, etc.). Accepts a ScriptFile object or an absolute path string. Returns undefined if parsing fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sampleFile | ScriptObject | no | ScriptFile object or absolute path string | Must point to valid audio file |

**Cross References:**
- `$API.Sampler.loadSampleMapFromJSON$`
- `$API.Sampler.getAudioWaveformContentAsBase64$`

---

## purgeMicPosition

**Signature:** `undefined purgeMicPosition(String micName, Integer shouldBePurged)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. callAsyncIfJobsPending.
**Minimal Example:** `{obj}.purgeMicPosition("Close", true);`

**Description:**
Purges or unpurges a mic position channel by name. The name must match the channel suffix string exactly (use `getMicPositionName()` to get valid names). Only works with multi-mic samplers.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| micName | String | no | Mic position name (channel suffix) | Non-empty, must match existing mic name |
| shouldBePurged | Integer | no | true to purge, false to unpurge | -- |

**Pitfalls:**
- Reports "Channel not found" if the name does not match any mic position exactly. Use `getMicPositionName()` to get valid names.
- Reports an error if called on a single-mic sampler without static matrix.

**Cross References:**
- `$API.Sampler.getMicPositionName$`
- `$API.Sampler.isMicPositionPurged$`
- `$API.Sampler.getNumMicPositions$`

---

## purgeSampleSelection

**Signature:** `undefined purgeSampleSelection(Array selection)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** killAllVoicesAndCall, allocates arrays.
**Minimal Example:** `{obj}.purgeSampleSelection(mySamples);`

**Description:**
Purges the specified Sample objects and unpurges all other samples. The selection array must contain Sample objects (from `createSelection()` etc.). All samples NOT in the array are unpurged; all samples IN the array are purged. Refreshes preload sizes and memory usage after purging.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| selection | Array | no | Array of Sample objects to purge | Must contain valid Sample objects |

**Pitfalls:**
- [BUG] The sampler pointer `s` is dereferenced before the null check (used in `ensureStorageAllocated` at line 4502 before the `s == nullptr` guard at line 4505). If the sampler is invalid, this causes undefined behavior.
- [BUG] The error message incorrectly says "purgeMicPosition()" instead of "purgeSampleSelection()".
- Reports errors if the array contains duplicate Sample objects or samples not in the current sampler.

**Cross References:**
- `$API.Sampler.createSelection$`
- `$API.Sampler.createSelectionFromIndexes$`

---

## refreshInterface

**Signature:** `undefined refreshInterface()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. Sends async change messages.
**Minimal Example:** `{obj}.refreshInterface();`

**Description:**
Sends change notifications to update the sampler interface and sound pool display. Call after programmatically modifying sample properties to refresh the UI.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Sampler.setSoundPropertyForAllSamples$`
- `$API.Sampler.setSoundPropertyForSelection$`

---

## refreshRRMap

**Signature:** `undefined refreshRRMap()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `{obj}.refreshRRMap();`

**Description:**
Rebuilds the internal round robin group map. Must be called after loading a sample map and before using `getRRGroupsForMessage()`. Requires round robin to be disabled.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Reports a script error if round robin is still enabled. Call `enableRoundRobin(false)` first.

**Cross References:**
- `$API.Sampler.getRRGroupsForMessage$`
- `$API.Sampler.enableRoundRobin$`

---

## saveCurrentSampleMap

**Signature:** `Integer saveCurrentSampleMap(String relativePathWithoutXml)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O operations.
**Minimal Example:** `var ok = {obj}.saveCurrentSampleMap("MySaved/Map");`

**Description:**
Saves the current sample map to an XML file in the SampleMaps directory. The path is relative to the SampleMaps folder; the .xml extension is added automatically. Returns true on success, false if the sampler has no sounds or the save fails. Overwrites existing files.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| relativePathWithoutXml | String | no | Relative path within SampleMaps directory (no .xml extension) | Non-empty |

**Cross References:**
- `$API.Sampler.getSampleMapAsBase64$`
- `$API.Sampler.loadSampleMap$`
- `$API.Sampler.getCurrentSampleMapId$`

---

## selectSounds

**Signature:** `undefined selectSounds(String regexWildcard)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `{obj}.selectSounds(".*");`

**Description:**
Populates the legacy script selection by matching sample file names against a regex pattern. Use `getNumSelectedSounds()`, `getSoundProperty()`, and `setSoundProperty()` to work with the selection. Consider using the modern `createSelection()` API instead.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| regexWildcard | String | no | Regular expression to match against sample file names | Valid regex pattern |

**Cross References:**
- `$API.Sampler.getNumSelectedSounds$`
- `$API.Sampler.getSoundProperty$`
- `$API.Sampler.setSoundProperty$`
- `$API.Sampler.createSelection$`

---

## setActiveGroup

**Signature:** `undefined setActiveGroup(Integer activeGroupIndex)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setActiveGroup(2);`

**Description:**
Sets a single active round robin group. Delegates to `setActiveGroupForEventId(-1, activeGroupIndex)` for global group setting. Round robin must be disabled first.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| activeGroupIndex | Integer | no | 1-based RR group index | 1 to RRGroupAmount |

**Pitfalls:**
- Requires `enableRoundRobin(false)` first. Reports a script error if round robin is enabled.
- Reports an error if the group index is invalid.

**Cross References:**
- `$API.Sampler.setActiveGroupForEventId$`
- `$API.Sampler.enableRoundRobin$`
- `$API.Sampler.setMultiGroupIndex$`

---

## setActiveGroupForEventId

**Signature:** `undefined setActiveGroupForEventId(Integer eventId, Integer activeGroupIndex)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** When eventId != -1, must be called from audio thread (onNoteOn callback).
**Minimal Example:** `{obj}.setActiveGroupForEventId(Message.getEventId(), 2);`

**Description:**
Sets the active round robin group for a specific event or globally. Pass -1 for global group setting. When using a specific event ID (!= -1), must be called from the audio thread (onNoteOn callback).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID, or -1 for global | -1 or valid event ID |
| activeGroupIndex | Integer | no | 1-based RR group index | 1 to RRGroupAmount |

**Pitfalls:**
- Requires `enableRoundRobin(false)` first. Reports a script error if round robin is enabled.
- When eventId != -1, reports "This method is only available in the onNoteOnCallback" if called outside the audio thread.

**Cross References:**
- `$API.Sampler.setActiveGroup$`
- `$API.Sampler.getActiveRRGroupForEventId$`
- `$API.Sampler.enableRoundRobin$`

---

## setAllowReleaseStart

**Signature:** `Integer setAllowReleaseStart(Integer eventId, Integer shouldBeAllowed)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ok = {obj}.setAllowReleaseStart(Message.getEventId(), true);`

**Description:**
Enables or disables release start for a specific event. Returns true on success. Requires HISE_SAMPLER_ALLOW_RELEASE_START preprocessor (enabled by default).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID | Valid event ID |
| shouldBeAllowed | Integer | no | true to allow release start, false to disallow | -- |

**Cross References:**
- `$API.Sampler.getReleaseStartOptions$`
- `$API.Sampler.setReleaseStartOptions$`

**Related Preprocessors:**
HISE_SAMPLER_ALLOW_RELEASE_START

---

## setAttribute

**Signature:** `undefined setAttribute(Number index, Number newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setAttribute({obj}.PreloadSize, 16384);`

**Description:**
Sets a sampler module attribute (processor parameter) by index. Uses the standard attribute notification type for proper UI/state updates.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Number | yes | Parameter index | Valid parameter index |
| newValue | Number | yes | New parameter value | Depends on parameter |

**Cross References:**
- `$API.Sampler.getAttribute$`
- `$API.Sampler.getAttributeId$`
- `$API.Sampler.getNumAttributes$`

---

## setGUISelection

**Signature:** `undefined setGUISelection(Array sampleList, Integer addToSelection)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. USE_BACKEND only. MessageManagerLock.
**Minimal Example:** `{obj}.setGUISelection(mySamples, false);`

**Description:**
Sets the sample editor GUI selection to the given array of Sample objects. Only works in the HISE IDE (USE_BACKEND). Silently does nothing in exported plugins.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sampleList | Array | no | Array of Sample objects | Must contain Sample objects |
| addToSelection | Integer | no | true to add to existing selection, false to replace | -- |

**Cross References:**
- `$API.Sampler.createListFromGUISelection$`
- `$API.Sampler.createSelection$`

**Related Preprocessors:**
USE_BACKEND

---

## setMultiGroupIndex

**Signature:** `undefined setMultiGroupIndex(var groupIndex, Integer enabled)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setMultiGroupIndex([1, 2, 3], true);`

**Description:**
Enables or disables one or more round robin groups simultaneously. Accepts a single group index (Integer), an array of group indices, or a MidiList object for bulk operations. Delegates to `setMultiGroupIndexForEventId(-1, ...)` for global state. Round robin must be disabled first.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| groupIndex | NotUndefined | no | Single group index, array of indices, or MidiList | Valid group indices |
| enabled | Integer | no | true to enable the group(s), false to disable | -- |

**Pitfalls:**
- Requires `enableRoundRobin(false)` first. Reports a script error if round robin is enabled.

**Cross References:**
- `$API.Sampler.setMultiGroupIndexForEventId$`
- `$API.Sampler.setActiveGroup$`
- `$API.Sampler.enableRoundRobin$`

---

## setMultiGroupIndexForEventId

**Signature:** `undefined setMultiGroupIndexForEventId(Integer eventId, var groupIndex, Integer enabled)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setMultiGroupIndexForEventId(Message.getEventId(), [1, 2], true);`

**Description:**
Enables or disables one or more round robin groups for a specific event or globally. Accepts a single group index, an array of group indices, or a MidiList object. Pass -1 for eventId for global state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID, or -1 for global | -1 or valid event ID |
| groupIndex | NotUndefined | no | Single group index, array of indices, or MidiList | Valid group indices |
| enabled | Integer | no | true to enable the group(s), false to disable | -- |

**Pitfalls:**
- Requires `enableRoundRobin(false)` first. Reports a script error if round robin is enabled.

**Cross References:**
- `$API.Sampler.setMultiGroupIndex$`
- `$API.Sampler.setActiveGroupForEventId$`
- `$API.Sampler.enableRoundRobin$`

---

## setRRGroupVolume

**Signature:** `undefined setRRGroupVolume(Integer groupIndex, Integer gainInDecibels)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setRRGroupVolume(1, -6);`

**Description:**
Sets the volume for a round robin group in decibels. Pass -1 as group index to set the volume for the currently active group. The dB value is converted to linear gain internally.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| groupIndex | Integer | no | 1-based RR group index, or -1 for active group | -1 or valid group index |
| gainInDecibels | Integer | no | Volume in decibels | Negative for attenuation |

**Cross References:**
- `$API.Sampler.setActiveGroup$`
- `$API.Sampler.getNumActiveGroups$`
- `$API.Sampler.enableRoundRobin$`

---

## setReleaseStartOptions

**Signature:** `undefined setReleaseStartOptions(JSON data)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates ReleaseStartOptions object.
**Minimal Example:** `{obj}.setReleaseStartOptions({"ReleaseFadeTime": 2048, "GainMatchingMode": "Volume"});`

**Description:**
Sets the release start options from a JSON object. Requires HISE_SAMPLER_ALLOW_RELEASE_START preprocessor (enabled by default).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| data | JSON | no | Configuration object with release start properties | See getReleaseStartOptions for schema |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| ReleaseFadeTime | Integer | Fade time in samples (0-44100, default 4096) |
| FadeGamma | Double | Gamma curve (clamped 0.0-2.0 in fromJSON, default 1.0) |
| UseAscendingZeroCrossing | Integer | Use ascending zero crossing (default false) |
| GainMatchingMode | String | "None", "Volume", or "Offset" |
| PeakSmoothing | Double | Peak smoothing factor (default 0.96) |

**Cross References:**
- `$API.Sampler.getReleaseStartOptions$`
- `$API.Sampler.setAllowReleaseStart$`

**Related Preprocessors:**
HISE_SAMPLER_ALLOW_RELEASE_START

---

## setSortByRRGroup

**Signature:** `undefined setSortByRRGroup(Integer shouldSort)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `{obj}.setSortByRRGroup(true);`

**Description:**
Enables or disables sorting samples by round robin group for optimized voice start. Recommended for large sample sets (>20,000 samples) to improve performance. When enabled, activates the `GroupedRoundRobinCollector` which pre-sorts sounds into groups.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldSort | Integer | no | true to enable group sorting, false to disable | -- |

**Cross References:**
- `$API.Sampler.enableRoundRobin$`
- `$API.Sampler.setActiveGroup$`

---

## setSoundProperty

**Signature:** `undefined setSoundProperty(Integer soundIndex, Integer propertyIndex, var newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `{obj}.setSoundProperty(0, Sampler.Volume, -3);`

**Description:**
Sets a sound property for a specific sound in the legacy script selection. Note the parameter order: soundIndex first, then propertyIndex (opposite of `getSoundProperty`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| soundIndex | Integer | no | Index into the legacy script selection | 0 to getNumSelectedSounds()-1 |
| propertyIndex | Integer | no | Sound property index (use Sampler constants) | 1-24 (SampleIds range) |
| newValue | NotUndefined | no | New property value | Depends on property |

**Pitfalls:**
- Parameter order is reversed compared to `getSoundProperty(propertyIndex, soundIndex)`. This inconsistency can cause hard-to-diagnose bugs.

**Cross References:**
- `$API.Sampler.getSoundProperty$`
- `$API.Sampler.selectSounds$`
- `$API.Sampler.setSoundPropertyForSelection$`
- `$API.Sampler.setSoundPropertyForAllSamples$`

---

## setSoundPropertyForAllSamples

**Signature:** `undefined setSoundPropertyForAllSamples(Integer propertyIndex, var newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. callAsyncIfJobsPending.
**Minimal Example:** `{obj}.setSoundPropertyForAllSamples(Sampler.Volume, 0);`

**Description:**
Sets a sound property to the same value for ALL samples in the sampler (not just the selection). Iterates all sounds using `SoundIterator`. Uses async execution if jobs are pending.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyIndex | Integer | no | Sound property index (use Sampler constants) | 1-24 (SampleIds range) |
| newValue | NotUndefined | no | New property value | Depends on property |

**Cross References:**
- `$API.Sampler.setSoundPropertyForSelection$`
- `$API.Sampler.setSoundProperty$`

---

## setSoundPropertyForSelection

**Signature:** `undefined setSoundPropertyForSelection(Integer propertyId, var newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD. callAsyncIfJobsPending.
**Minimal Example:** `{obj}.setSoundPropertyForSelection(Sampler.Volume, -6);`

**Description:**
Sets a sound property to the same value for all sounds in the legacy script selection (populated by `selectSounds()`). Uses async execution if jobs are pending.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyId | Integer | no | Sound property index (use Sampler constants) | 1-24 (SampleIds range) |
| newValue | NotUndefined | no | New property value | Depends on property |

**Cross References:**
- `$API.Sampler.selectSounds$`
- `$API.Sampler.setSoundPropertyForAllSamples$`
- `$API.Sampler.setSoundProperty$`

---

## setTimestretchOptions

**Signature:** `undefined setTimestretchOptions(JSON newOptions)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates TimestretchOptions, modifies sampler state.
**Minimal Example:** `{obj}.setTimestretchOptions({"Mode": "VoiceStart", "Tonality": 0.5});`

**Description:**
Sets the timestretch configuration from a JSON object.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newOptions | JSON | no | Configuration object with timestretch properties | See getTimestretchOptions for schema |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Disabled" | No timestretching applied |
| "VoiceStart" | New voices use current ratio, active voices keep theirs |
| "TimeVariant" | All active voices use the same ratio (updated in real-time) |
| "TempoSynced" | Ratio automatically calculated from sample length and current tempo |

**Cross References:**
- `$API.Sampler.getTimestretchOptions$`
- `$API.Sampler.setTimestretchRatio$`

---

## setTimestretchRatio

**Signature:** `undefined setTimestretchRatio(Double newRatio)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies sampler timestretch state.
**Minimal Example:** `{obj}.setTimestretchRatio(1.5);`

**Description:**
Sets the timestretch ratio. A value of 1.0 means original speed, 2.0 means double speed, 0.5 means half speed. Timestretch mode must be enabled via `setTimestretchOptions()` first.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newRatio | Double | no | Timestretch ratio | Positive value, 1.0 = original speed |

**Cross References:**
- `$API.Sampler.setTimestretchOptions$`
- `$API.Sampler.getTimestretchOptions$`

---

## setUseStaticMatrix

**Signature:** `undefined setUseStaticMatrix(Integer shouldUseStaticMatrix)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** WARN_IF_AUDIO_THREAD.
**Minimal Example:** `{obj}.setUseStaticMatrix(true);`

**Description:**
Enables or disables the static routing matrix for this sampler. When enabled, the sampler uses a fixed channel routing matrix instead of the dynamic mic position system.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseStaticMatrix | Integer | no | true to enable static matrix, false for dynamic mic routing | -- |

**Cross References:**
- `$API.Sampler.getNumMicPositions$`
- `$API.Sampler.purgeMicPosition$`
