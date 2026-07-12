# AudioSampleProcessor -- Method Documentation

## exists

**Signature:** `bool exists()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.exists();`

**Description:**
Returns whether the underlying processor reference is still valid. Returns false if the processor has been deleted (e.g., removed by Builder) or if the handle was constructed with a null reference (e.g., when `Synth.getAudioSampleProcessor()` could not find the specified module).

**Parameters:**

None.

## getAttribute

**Signature:** `float getAttribute(int parameterIndex)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var val = {obj}.getAttribute({obj}.Gain);`

**Description:**
Returns the current value of the module parameter at the given index. Use the dynamic constants exposed on the handle (e.g., `asp.Gain`, `asp.SyncMode`) as the index. The available constants depend on the wrapped processor type.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Number | no | Index of the parameter to query. Use the dynamic constants on the handle. | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.AudioSampleProcessor.getSampleRange$`
- `$API.AudioSampleProcessor.getSampleStart$`
- `$API.AudioSampleProcessor.setSampleRange$`

## getNumAttributes

**Signature:** `int getNumAttributes()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var num = {obj}.getNumAttributes();`

**Description:**
Returns the total number of parameters exposed by the wrapped processor module. The count depends on the concrete module type (e.g., AudioLooper has 10 parameters, ConvolutionEffect has 10).

**Parameters:**

None.

**Cross References:**
- `$API.AudioSampleProcessor.getAttribute$`
- `$API.AudioSampleProcessor.getAttributeId$`
- `$API.AudioSampleProcessor.getAttributeIndex$`

## getSampleLength

**Signature:** `int getSampleLength()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var len = {obj}.getSampleLength();`

**Description:**
Returns the length of the current sample range in samples. This is the difference between the range end and range start (`getSampleRange()[1] - getSampleRange()[0]`), not the total file length. Returns 0 if no file is loaded or the handle is invalid.

**Parameters:**

None.

**Cross References:**
- `$API.AudioSampleProcessor.getSampleRange$`
- `$API.AudioSampleProcessor.getTotalLengthInSamples$`

## getSampleRange

**Signature:** `var getSampleRange()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an Array for the return value.
**Minimal Example:** `var range = {obj}.getSampleRange();`

**Description:**
Returns the active sample playback range as a two-element array `[start, end]` in samples. The range defines which portion of the loaded audio file is used for playback. Returns undefined if no file is loaded.

**Parameters:**

None.

**Cross References:**
- `$API.AudioSampleProcessor.setSampleRange$`
- `$API.AudioSampleProcessor.getSampleLength$`
- `$API.AudioSampleProcessor.getSampleStart$`

## getSampleStart

**Signature:** `var getSampleStart()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var start = {obj}.getSampleStart();`

**Description:**
Returns the start position of the current sample range in samples. Equivalent to `getSampleRange()[0]` but avoids the Array allocation. Returns 0 if no file is loaded or the handle is invalid.

**Parameters:**

None.

**Cross References:**
- `$API.AudioSampleProcessor.getSampleRange$`
- `$API.AudioSampleProcessor.setSampleRange$`

## getTotalLengthInSamples

**Signature:** `var getTotalLengthInSamples()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var total = {obj}.getTotalLengthInSamples();`

**Description:**
Returns the total length of the loaded audio file in samples, regardless of the current sample range. This is the full file length, not the active playback range. Returns undefined if no file is loaded.

**Parameters:**

None.

**Cross References:**
- `$API.AudioSampleProcessor.getSampleLength$`
- `$API.AudioSampleProcessor.setSampleRange$`

## getAttributeId

**Signature:** `String getAttributeId(int parameterIndex)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return value involves atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getAttributeId(0);`

**Description:**
Returns the name of the module parameter at the given index as a string. Use this to discover parameter names at runtime when the dynamic constants are not known ahead of time. Returns an empty string if the handle is invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Number | no | Index of the parameter to query. | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.AudioSampleProcessor.getAttributeIndex$`
- `$API.AudioSampleProcessor.getAttribute$`
- `$API.AudioSampleProcessor.getNumAttributes$`

## getAttributeIndex

**Signature:** `int getAttributeIndex(String parameterId)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String parameter involves atomic ref-count operations.
**Minimal Example:** `var idx = {obj}.getAttributeIndex("Gain");`

**Description:**
Returns the parameter index for the given parameter name string. This is the reverse lookup of `getAttributeId()` -- given a name like `"Gain"`, it returns the integer index usable with `getAttribute()` and `setAttribute()`. Returns -1 if the handle is invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterId | String | no | The parameter name to look up. | Must match a valid parameter name for the wrapped processor |

**Cross References:**
- `$API.AudioSampleProcessor.getAttributeId$`
- `$API.AudioSampleProcessor.getAttribute$`
- `$API.AudioSampleProcessor.setAttribute$`

## getAudioFile

**Signature:** `ScriptAudioFile getAudioFile(int slotIndex)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ScriptAudioFile object on the heap.
**Minimal Example:** `var af = {obj}.getAudioFile(0);`

**Description:**
Returns an `AudioFile` scripting object for the audio file data at the given slot index. AudioSampleProcessor modules have exactly one audio file slot (index 0). The returned `AudioFile` object provides access to the sample data buffer, content change callbacks, and direct buffer manipulation through the complex data API.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| slotIndex | Number | no | The audio file slot index to access. | Use 0 for AudioSampleProcessor (single slot) |

**Pitfalls:**
- [BUG] The `slotIndex` parameter is not bounds-checked. AudioSampleProcessor modules always have exactly one slot (index 0). Passing any other index creates an AudioFile handle pointing to a non-existent slot, which may produce undefined behavior.

**Cross References:**
- `$API.AudioSampleProcessor.setFile$`
- `$API.AudioSampleProcessor.getFilename$`

## getFilename

**Signature:** `String getFilename()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return value involves atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getFilename();`

**Description:**
Returns the pool reference string of the currently loaded audio file (e.g., `{PROJECT_FOLDER}my_loop.wav` or `{EXP::myExpansion}file.wav`). This is the same format used by `setFile()`, not a filesystem path. Returns an empty string if no file is loaded or the handle is invalid.

**Parameters:**

None.

**Cross References:**
- `$API.AudioSampleProcessor.setFile$`
- `$API.AudioSampleProcessor.getAudioFile$`

## getLoopRange

**Signature:** `var getLoopRange(bool subtractStart)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an Array for the return value.
**Minimal Example:** `var loop = {obj}.getLoopRange(false);`

**Description:**
Returns the loop range as a two-element array `[start, end]` in samples. When `subtractStart` is true, the returned positions are relative to the current sample range start (i.e., the sample range start is subtracted from both values). When false, positions are absolute within the audio file. Returns undefined if no file is loaded or the handle is invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| subtractStart | Number | no | Whether to make the loop range relative to the current sample range start. | Boolean (0 or 1) |

**Cross References:**
- `$API.AudioSampleProcessor.getSampleRange$`
- `$API.AudioSampleProcessor.setSampleRange$`
- `$API.AudioSampleProcessor.getSampleStart$`

## isBypassed

**Signature:** `bool isBypassed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bp = {obj}.isBypassed();`

**Description:**
Returns whether the wrapped processor module is currently bypassed. Returns false if the handle is invalid.

**Parameters:**

None.

**Cross References:**
- `$API.AudioSampleProcessor.setBypassed$`

## setAttribute

**Signature:** `void setAttribute(int parameterIndex, float newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Sends an asynchronous notification that triggers ValueTree property changes and listener callbacks.
**Minimal Example:** `{obj}.setAttribute({obj}.Gain, 0.5);`

**Description:**
Sets the value of the module parameter at the given index. Uses the dynamic constants exposed on the handle (e.g., `asp.Gain`, `asp.SyncMode`) as the index. Sends an asynchronous change notification to update the UI and any connected listeners.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Number | no | Index of the parameter to set. Use the dynamic constants on the handle. | 0 to getNumAttributes()-1 |
| newValue | Number | no | The new parameter value. | Range depends on the parameter |

**Cross References:**
- `$API.AudioSampleProcessor.getAttribute$`
- `$API.AudioSampleProcessor.getAttributeId$`

## setBypassed

**Signature:** `void setBypassed(bool shouldBeBypassed)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Sends a synchronous notification and dispatches a ProcessorChangeEvent.
**Minimal Example:** `{obj}.setBypassed(true);`

**Description:**
Sets the bypass state of the wrapped processor module. When bypassed, the module's processing is skipped. Also dispatches a `ProcessorChangeEvent::Bypassed` notification to update the UI.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Number | no | Whether to bypass the module. | Boolean (0 or 1) |

**Cross References:**
- `$API.AudioSampleProcessor.isBypassed$`

## setFile

**Signature:** `void setFile(String fileName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Loads audio data from the pool, involves I/O and complex processing.
**Minimal Example:** `{obj}.setFile("{PROJECT_FOLDER}my_loop.wav");`

**Description:**
Loads an audio file from the HISE audio file pool. The `fileName` parameter is a pool reference string, not a filesystem path. Use `{PROJECT_FOLDER}` as a wildcard for the project's AudioFiles directory (e.g., `{PROJECT_FOLDER}my_loop.wav`). Expansion files use `{EXP::expansionName}` syntax. In the HISE IDE (backend), `Engine.loadAudioFilesIntoPool()` must be called before this method for `{PROJECT_FOLDER}` references. In exported plugins, audio files are embedded and no pool preloading is needed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | Pool reference string for the audio file. | Must use HISE pool reference format |

**Cross References:**
- `$API.AudioSampleProcessor.getFilename$`
- `$API.AudioSampleProcessor.getAudioFile$`
- `$API.Engine.loadAudioFilesIntoPool$`

**Example:**
```javascript:set-file-with-range
// Title: Loading an audio file and setting a playback range
const var asp = Synth.getAudioSampleProcessor("AudioLooper1");

// Load a file from the project's AudioFiles folder
asp.setFile("{PROJECT_FOLDER}my_loop.wav");

// Set the playback range to the full file
asp.setSampleRange(0, asp.getTotalLengthInSamples());
```
```json:testMetadata:set-file-with-range
{
  "testable": false,
  "skipReason": "Requires an AudioLooper module and an audio file in the project pool."
}
```

## setSampleRange

**Signature:** `void setSampleRange(int start, int end)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Sends a range change notification to the audio file buffer.
**Minimal Example:** `{obj}.setSampleRange(0, 44100);`

**Description:**
Sets the active sample playback range in samples. The `start` and `end` values define the portion of the loaded audio file that will be used for playback. The range is applied to the underlying `MultiChannelAudioBuffer` at slot 0.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| start | Number | no | Start position in samples (inclusive). | 0 to total file length |
| end | Number | no | End position in samples (exclusive). | start to total file length |

**Cross References:**
- `$API.AudioSampleProcessor.getSampleRange$`
- `$API.AudioSampleProcessor.getSampleLength$`
- `$API.AudioSampleProcessor.getTotalLengthInSamples$`
