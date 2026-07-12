## getContent

**Signature:** `Array getContent()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates Array and Buffer objects (heap allocation).
**Minimal Example:** `var channels = {obj}.getContent();`

**Description:**
Returns the audio data as an Array of Buffer objects, one per channel. The returned buffers contain the current range data (as set by `setRange`), not the full original file content. If no audio is loaded, returns an empty array.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns data for the current range only. After `setRange(1000, 2000)`, the returned buffers contain 1000 samples, not the full file. Use `setRange(0, getTotalLengthInSamples())` first to access the full file content.

**Cross References:**
- `$API.AudioFile.getNumSamples$`
- `$API.AudioFile.setRange$`
- `$API.AudioFile.loadBuffer$`

---

## getCurrentlyDisplayedIndex

**Signature:** `double getCurrentlyDisplayedIndex()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var idx = {obj}.getCurrentlyDisplayedIndex();`

**Description:**
Returns the current display position index as a floating-point value. This is the playback position reported by the audio processor using the buffer, updated asynchronously. Useful for syncing UI display elements with playback progress.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.AudioFile.setDisplayCallback$`

---

## getCurrentlyLoadedFile

**Signature:** `String getCurrentlyLoadedFile()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return, atomic ref-count operations.
**Minimal Example:** `var ref = {obj}.getCurrentlyLoadedFile();`

**Description:**
Returns the reference string of the currently loaded audio file. This is a HISE pool reference string (e.g. `{PROJECT_FOLDER}audiofile.wav`), not a filesystem path. Returns an empty string if no file is loaded.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns a HISE pool reference string, not a filesystem path. The format depends on the data provider (typically `{PROJECT_FOLDER}filename.wav` for pool-based files).

**Cross References:**
- `$API.AudioFile.loadFile$`

---

## getLoopRange

**Signature:** `Array getLoopRange(Integer subtractStart)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates Array (heap allocation).
**Minimal Example:** `var loop = {obj}.getLoopRange(0);`

**Description:**
Returns the loop range as a two-element array `[start, end]` in sample positions. If `subtractStart` is true (non-zero), the returned positions are relative to the current range start (as set by `setRange`). If false, they are absolute positions within the original file. Returns `undefined` if no audio is loaded.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| subtractStart | Integer | no | If true, subtract the current range start from the loop positions | 0 or 1 |

**Cross References:**
- `$API.AudioFile.setRange$`
- `$API.AudioFile.getRange$`
- `$API.AudioFile.loadFile$`
- `$API.AudioFile.loadBuffer$`

---

## getNumSamples

**Signature:** `int getNumSamples()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var num = {obj}.getNumSamples();`

**Description:**
Returns the number of samples in the current range. After calling `setRange`, this returns the range size, not the total file length. Returns 0 if no audio is loaded.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns the current range size, not the total file length. Use `getTotalLengthInSamples` for the full original file length.

**Cross References:**
- `$API.AudioFile.getTotalLengthInSamples$`
- `$API.AudioFile.setRange$`

---

## getRange

**Signature:** `Array getRange()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates Array (heap allocation).
**Minimal Example:** `var range = {obj}.getRange();`

**Description:**
Returns the current sample range as a two-element array `[start, end]`. The range defines which portion of the original audio file is active. Returns `undefined` if no audio is loaded.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.AudioFile.setRange$`
- `$API.AudioFile.getNumSamples$`
- `$API.AudioFile.getTotalLengthInSamples$`

---

## getSampleRate

**Signature:** `double getSampleRate()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var sr = {obj}.getSampleRate();`

**Description:**
Returns the sample rate of the loaded audio file in Hz. Returns 0.0 if no audio is loaded.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.AudioFile.loadFile$`
- `$API.AudioFile.loadBuffer$`
- `$API.AudioFile.getTotalLengthInSamples$`
- `$API.AudioFile.getNumSamples$`

---

## getTotalLengthInSamples

**Signature:** `int getTotalLengthInSamples()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var total = {obj}.getTotalLengthInSamples();`

**Description:**
Returns the total length of the original audio file in samples, regardless of the current range set by `setRange`. Returns `undefined` if no audio is loaded.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.AudioFile.getNumSamples$`
- `$API.AudioFile.setRange$`

---

## linkTo

**Signature:** `undefined linkTo(ScriptObject otherAudioFile)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies listener registrations and data holder linkage.
**Minimal Example:** `{obj}.linkTo(otherAudioFile);`

**Description:**
Links this AudioFile's data slot to another AudioFile's data source. After linking, both references share the same underlying audio buffer. Changes to one are reflected in the other. The data type must match -- linking an AudioFile to a Table or SliderPackData produces a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherAudioFile | ScriptObject | no | Another AudioFile reference to link to | Must be same data type |

**Pitfalls:**
- Type mismatch (e.g. linking an AudioFile to a Table) produces a script error "Type mismatch". Passing a non-data-reference object produces "Not a data object".

**Cross References:**
- `$API.AudioFile.getContent$`
- `$API.AudioFile.loadFile$`
- `$API.AudioFile.loadBuffer$`

---

## loadBuffer

**Signature:** `undefined loadBuffer(AudioData bufferData, Double sampleRate, Array loopRange)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies audio buffer, acquires write lock.
**Minimal Example:** `{obj}.loadBuffer(myBuffer, 44100.0, [0, 1000]);`

**Description:**
Loads audio data from script Buffer objects into the AudioFile. Accepts either a single Buffer (mono) or an Array of Buffers (multi-channel, one Buffer per channel). The sample rate is set explicitly. The loop range is optional -- pass a two-element array `[start, end]` to set loop points, or an empty array to skip.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| bufferData | AudioData | no | A single Buffer (mono) or Array of Buffers (multi-channel) | -- |
| sampleRate | Double | no | Sample rate of the audio data in Hz | > 0 |
| loopRange | Array | no | Two-element array [start, end] for loop points, or empty | 2 elements or empty |

**Pitfalls:**
- [BUG] When passing an Array of Buffers, if any element is not a valid Buffer, the corresponding channel pointer is uninitialized, which can cause a crash or corrupt data. All elements in the array must be valid Buffer objects.
- [BUG] When passing an Array of Buffers with different lengths, the sample count is taken from the last valid Buffer. Shorter buffers will be read past their end, causing undefined behavior. All Buffers in the array must have the same length.

**Cross References:**
- `$API.AudioFile.loadFile$`
- `$API.AudioFile.getContent$`
- `$API.AudioFile.getLoopRange$`

**Example:**
```javascript:load-buffer-mono
// Title: Loading a programmatically generated mono buffer
const var af = Engine.createAndRegisterAudioFile(0);
const var buf = Buffer.create(128);

// Fill with a sine wave
for (i = 0; i < 128; i++)
    buf[i] = Math.sin(2.0 * Math.PI * 440.0 * i / 44100.0);

af.loadBuffer(buf, 44100.0, []);
```
```json:testMetadata:load-buffer-mono
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "af.getNumSamples()", "value": 128},
    {"type": "REPL", "expression": "af.getSampleRate()", "value": 44100.0}
  ]
}
```

---

## loadFile

**Signature:** `undefined loadFile(String filePath)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** File I/O, acquires write lock on the audio buffer.
**Minimal Example:** `{obj}.loadFile("{PROJECT_FOLDER}audiofile.wav");`

**Description:**
Loads an audio file from a HISE pool reference string. The reference format is typically `{PROJECT_FOLDER}filename.wav` for files in the project's AudioFiles folder. Pass an empty string to clear the buffer. The loading is performed synchronously with a write lock on the underlying buffer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| filePath | String | no | HISE pool reference string | -- |

**Pitfalls:**
- The parameter name "filePath" is misleading -- it accepts a HISE pool reference string (e.g. `{PROJECT_FOLDER}audio.wav`), not a filesystem path.

**Cross References:**
- `$API.AudioFile.getCurrentlyLoadedFile$`
- `$API.AudioFile.loadBuffer$`

---

## setContentCallback

**Signature:** `undefined setContentCallback(Function contentFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates WeakCallbackHolder (heap allocation).
**Minimal Example:** `{obj}.setContentCallback(onContentChanged);`

**Callback Signature:** contentFunction()

**Description:**
Registers a callback function that fires when the audio content changes (file loaded, buffer modified, range changed). The callback takes no arguments; `this` inside the callback points to the AudioFile that changed. Use `inline function` syntax for the callback to ensure audio-thread safety.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| contentFunction | Function | yes | Callback to execute on content change | `this` = the AudioFile |

**Cross References:**
- `$API.AudioFile.setDisplayCallback$`
- `$API.AudioFile.loadFile$`
- `$API.AudioFile.update$`

**Example:**
```javascript:content-callback-setup
// Title: Registering a content change callback
const var af = Engine.createAndRegisterAudioFile(0);

reg callCount = 0;

inline function onContentChanged()
{
    // 'this' points to the AudioFile that changed
    Console.print("Content changed: " + this.getNumSamples() + " samples");
    callCount++;
};

af.setContentCallback(onContentChanged);

// --- test-only ---
const var trigBuf = Buffer.create(64);
af.loadBuffer(trigBuf, 44100.0, []);
// --- end test-only ---
```
```json:testMetadata:content-callback-setup
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 500, "expression": "callCount", "value": 1}
  ]
}
```

---

## setDisplayCallback

**Signature:** `undefined setDisplayCallback(Function displayFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates WeakCallbackHolder (heap allocation).
**Minimal Example:** `{obj}.setDisplayCallback(onDisplayUpdate);`

**Callback Signature:** displayFunction(displayIndex: double)

**Description:**
Registers a callback function that fires when the display index changes (e.g. during audio playback). The callback receives the current display position as a floating-point value. Use `inline function` syntax for the callback to ensure audio-thread safety.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| displayFunction | Function | yes | Callback to execute on display index change | Must accept 1 argument |

**Cross References:**
- `$API.AudioFile.setContentCallback$`
- `$API.AudioFile.getCurrentlyDisplayedIndex$`

**Example:**
```javascript:display-callback-setup
// Title: Registering a display position callback
const var af = Engine.createAndRegisterAudioFile(0);

inline function onDisplayUpdate(displayIndex)
{
    Console.print("Position: " + displayIndex);
};

af.setDisplayCallback(onDisplayUpdate);
```
```json:testMetadata:display-callback-setup
{
  "testable": false,
  "skipReason": "Display callback fires asynchronously during active audio playback, requires a running audio processor"
}
```

---

## setRange

**Signature:** `undefined setRange(Integer min, Integer max)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires write lock, copies buffer data for the sub-range.
**Minimal Example:** `{obj}.setRange(0, 44100);`

**Description:**
Sets the active sample range within the loaded audio file. The range is clamped to valid bounds (0 to total length). After calling this, `getContent` and `getNumSamples` reflect only the selected range. Setting a zero-length range or calling on an empty buffer clears the data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| min | Integer | no | Start position in samples (inclusive) | >= 0, clamped to valid range |
| max | Integer | no | End position in samples (exclusive) | <= total length, clamped |

**Cross References:**
- `$API.AudioFile.getRange$`
- `$API.AudioFile.getNumSamples$`
- `$API.AudioFile.getTotalLengthInSamples$`
- `$API.AudioFile.getContent$`

---

## update

**Signature:** `undefined update()`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Posts an asynchronous notification only, no allocation or lock.
**Minimal Example:** `{obj}.update();`

**Description:**
Sends an asynchronous content change notification to all registered listeners and content callbacks. Use this after modifying the audio data directly through Buffer objects obtained from `getContent` to trigger UI updates and listener notifications.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.AudioFile.getContent$`
- `$API.AudioFile.setContentCallback$`
- `$API.AudioFile.loadBuffer$`

