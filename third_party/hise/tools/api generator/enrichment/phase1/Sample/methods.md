# Sample -- Method Analysis

## deleteSample

**Signature:** `undefined deleteSample()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Schedules async voice-killing via killAllVoicesAndCall, involves lambda allocation and audio thread coordination.
**Minimal Example:** `{obj}.deleteSample();`

**Description:**
Removes this sample from the sampler's sample map. The removal is deferred -- voices are killed first via `killAllVoicesAndCall`, then the sound is removed from the sample map in the async callback. After deletion, this Sample object becomes invalid and any further method calls will throw "Sound does not exist".

The entire method body is guarded by the `HI_ENABLE_EXPANSION_EDITING` preprocessor flag. In builds without this flag (some frontend/export configurations), the method exists but does nothing.

**Parameters:**
(none)

**Pitfalls:**
- In builds without `HI_ENABLE_EXPANSION_EDITING`, this method is a silent no-op -- no error is reported but the sample is not removed.
- After calling deleteSample(), the Sample reference is invalidated. Any subsequent call on the same object throws "Sound does not exist".

**Cross References:**
- `$API.Sample.duplicateSample$`
- `$API.Sampler.createSelection$`

## duplicateSample

**Signature:** `Sample duplicateSample()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Kills voices, acquires sample lock, suspends audio processing, allocates new sample data, refreshes preload sizes.
**Minimal Example:** `var copy = {obj}.duplicateSample();`

**Description:**
Creates a deep copy of this sample within the same sampler. The method performs heavyweight thread synchronization: sets sync edit mode, suspends audio, kills all voices with a 1000ms timeout, busy-waits for audio to stop, acquires the sample lock, then copies the sample's ValueTree data. The new sample is added to the end of the sample map and preload sizes are refreshed. Returns a new Sample object pointing to the copy.

**Parameters:**
(none)

**Pitfalls:**
- [BUG] No `objectExists()` check -- if the underlying sound was deleted, this method dereferences a null pointer at `sound->getData()` instead of reporting "Sound does not exist".

**Cross References:**
- `$API.Sample.deleteSample$`

## get

**Signature:** `var get(int propertyIndex)`
**Return Type:** `var`
**Call Scope:** warning
**Call Scope Note:** String involvement when accessing FileName property -- atomic ref-count operations on String copy. Integer properties are safe.
**Minimal Example:** `var root = {obj}.get(Sample.Root);`

**Description:**
Returns the value of the specified sample property. For the FileName property (index 1), returns a String containing the audio file path. For all other properties, the underlying value is cast to int before returning, regardless of internal storage type.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyIndex | Integer | no | Property constant index | Use Sample.FileName through Sample.Reversed (1-23) |

**Pitfalls:**
- All non-FileName properties are cast to int on return. Volume (dB) and Pan (percentage) values are integers, not floats.

**Cross References:**
- `$API.Sample.set$`
- `$API.Sample.setFromJSON$`
- `$API.Sample.getRange$`

## getCustomProperties

**Signature:** `JSON getCustomProperties()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a DynamicObject on first call (heap allocation). Subsequent calls return the cached object.
**Minimal Example:** `var props = {obj}.getCustomProperties();`

**Description:**
Returns a mutable JSON object for attaching arbitrary key-value metadata to this sample. The object is lazily created on first access and reused on subsequent calls. This data is transient -- it exists only on the scripting wrapper and is NOT persisted when the sample map is saved or exported.

**Parameters:**
(none)

**Example:**
```javascript:custom-props-attach
// Title: Attaching transient metadata to samples
const var allSamples = Sampler.createSelectionFromIndexes(-1);

for (s in allSamples)
{
    local props = s.getCustomProperties();
    props.analyzed = true;
    props.category = "percussion";
}

// Later retrieval returns the same object
var firstProps = allSamples[0].getCustomProperties();
Console.print(firstProps.category);
```

```json:testMetadata:custom-props-attach
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

**Cross References:**
- `$API.Sample.get$`
- `$API.Sample.set$`
- `$API.Sample.setFromJSON$`

## loadIntoBufferArray

**Signature:** `Array loadIntoBufferArray()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates VariantBuffer objects and AudioFormatReaders for each mic position. Loads entire sample data into memory.
**Minimal Example:** `var buffers = {obj}.loadIntoBufferArray();`

**Description:**
Loads the complete audio data of this sample into an array of Buffer objects. Iterates over all multi-mic positions, creating an AudioFormatReader for each. For mono mic positions, adds one buffer; for stereo, adds two (left, right). Returns a flat array of all channel buffers across all mic positions. For a sample with 2 stereo mic positions, the returned array has 4 buffers: [mic1_L, mic1_R, mic2_L, mic2_R].

**Parameters:**
(none)

**Pitfalls:**
- [BUG] No `objectExists()` check -- if the underlying sound was deleted, this method dereferences a null pointer instead of reporting "Sound does not exist".
- Loads the entire sample into memory. For large samples or many mic positions, this allocates significant memory.

**Cross References:**
- `$API.Sample.replaceAudioFile$`

## refersToSameSample

**Signature:** `bool refersToSameSample(var otherSample)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var same = {obj}.refersToSameSample(otherSample);`

**Description:**
Returns true if both Sample objects refer to the same underlying ModulatorSamplerSound instance. This is a pointer identity check, not a property comparison. Useful for detecting whether two selections contain overlapping samples.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherSample | ScriptObject | no | Another Sample object to compare | Must be a Sample object |

**Pitfalls:**
- [BUG] The error message when a non-Sample argument is passed contains a typo: "refersToSampleSample" instead of "refersToSameSample".

**Cross References:**
- `$API.Sampler.createSelection$`

## replaceAudioFile

**Signature:** `bool replaceAudioFile(var audioData)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Writes to audio files on disk (I/O operations). No explicit thread safety guards -- caller must ensure no voices are playing.
**Minimal Example:** `var ok = {obj}.replaceAudioFile(bufferArray);`

**Description:**
Replaces the audio data of this sample's file(s) on disk with the provided buffer array. The array must contain one Buffer per channel across all mic positions, matching the structure returned by `loadIntoBufferArray()`. All buffers must have the same length. Returns true on success, false on write failure.

Validation order: checks sound exists, validates audioData is array, checks no mic position uses monolithic storage, counts total channels across mic positions, validates buffer count matches channel count, validates buffer lengths match, then writes per mic position.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| audioData | Array | no | Array of Buffer objects, one per channel | Buffer count must match total channels across mic positions |

**Pitfalls:**
- Cannot write to monolithic sample files (.ch1, .ch2, etc.) -- throws "Can't write to monolith files".
- [BUG] After reporting "channel length mismatch" or "Invalid channel data" errors (lines 2780, 2784, 2796), execution continues without returning in non-throwing builds. This can lead to null pointer dereference or writing with inconsistent buffer lengths.
- No explicit thread safety -- unlike duplicateSample and deleteSample, this method does not kill voices or acquire locks. The caller must ensure safe state.

**Cross References:**
- `$API.Sample.loadIntoBufferArray$`

## set

**Signature:** `undefined set(int propertyIndex, var newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to setSampleProperty which operates on ValueTree data with notifications, string lookups, and potential cascading property adjustments.
**Minimal Example:** `{obj}.set(Sample.Root, 60);`

**Description:**
Sets the specified sample property to the given value. The value is automatically clipped to the valid range for the property (use `getRange()` to query). Setting certain properties may trigger cascading adjustments to dependent properties to maintain internal consistency (e.g., setting SampleStart may adjust LoopXFade, LoopStart, SampleStartMod).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyIndex | Integer | no | Property constant index | Use Sample.FileName through Sample.Reversed (1-23) |
| newValue | var | no | New value for the property | Automatically clipped to valid range |

**Cross References:**
- `$API.Sample.get$`
- `$API.Sample.getRange$`
- `$API.Sample.setFromJSON$`

## setFromJSON

**Signature:** `undefined setFromJSON(var object)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Iterates properties calling setSampleProperty -- same ValueTree operations as set().
**Minimal Example:** `{obj}.setFromJSON({"Root": 60, "HiKey": 72, "LoKey": 48});`

**Description:**
Sets multiple sample properties from a JSON object. Property keys must use the SampleIds identifier names (e.g., "Root", "HiKey", "LoVel"), not integer indices. Each property is set individually via setSampleProperty, so cascading range adjustments apply per property in iteration order.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| object | JSON | no | Object with property name/value pairs | Keys must be SampleIds identifier strings |

**Pitfalls:**
- [BUG] Non-object input (arrays, strings, numbers) is silently ignored -- no error is reported and no properties are changed.

**Example:**
```javascript:set-from-json-batch
// Title: Setting multiple sample properties at once
const var allSamples = Sampler.createSelectionFromIndexes(-1);

for (s in allSamples)
{
    s.setFromJSON({
        "Volume": -6,
        "Pan": 0,
        "LoVel": 0,
        "HiVel": 127
    });
}
```

```json:testMetadata:set-from-json-batch
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

**Cross References:**
- `$API.Sample.set$`
- `$API.Sample.get$`
- `$API.Sample.getRange$`

## getId

**Signature:** `String getId(int id)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement -- returns a String from Identifier::toString(), which involves atomic ref-count operations on the StringHolder.
**Minimal Example:** `var name = {obj}.getId(Sample.Root);`

**Description:**
Returns the string identifier name for a property index. For example, passing `Sample.Root` (value 2) returns `"Root"`. This is the reverse mapping of the bracket-access string resolution -- given an integer constant, it produces the SampleIds identifier name. Useful with `setFromJSON` to construct property objects programmatically from integer indices.

Note: This method is declared in the class header and present in the base JSON, but has no Wrapper struct entry and no ADD_API_METHOD registration. It may be exposed through an alternative mechanism or may not be callable in all HISE builds. The implementation is a direct array lookup: `sampleIds[id].toString()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | Integer | no | Property constant index | Use Sample.FileName through Sample.Reversed (1-23) |

**Pitfalls:**
- [BUG] No bounds checking on the index parameter. Passing an index outside the valid range (0-23) will access out-of-bounds memory in the sampleIds array.
- No `objectExists()` check -- unlike most other Sample methods, this does not verify the underlying sound is still valid before executing. However, since the method only accesses the `sampleIds` array (which belongs to the wrapper, not the sound), this is not a practical concern.

**Cross References:**
- `$API.Sample.get$`
- `$API.Sample.set$`
- `$API.Sample.setFromJSON$`

## getRange

**Signature:** `var getRange(int propertyIndex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an Array with two elements on each call. Also delegates to ModulatorSamplerSound::getPropertyRange() which accesses ValueTree properties.
**Minimal Example:** `var range = {obj}.getRange(Sample.Root);`

**Description:**
Returns a two-element array `[start, end]` representing the valid value range for the specified property. Ranges are dynamic and depend on the current values of related properties. For example, the HiKey range starts at the current LoKey value, and the SampleStart range ends at SampleEnd or LoopStart. Use this to validate or constrain UI controls before calling `set()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyIndex | Integer | no | Property constant index | Use Sample.FileName through Sample.Reversed (1-23) |

**Example:**
```javascript:dynamic-range-query
// Title: Querying dynamic property ranges before setting values
const var allSamples = Sampler.createSelectionFromIndexes(-1);
const var s = allSamples[0];

// Get the valid range for LoKey (depends on current HiKey)
var loKeyRange = s.getRange(Sample.LoKey);
Console.print("LoKey valid range: " + loKeyRange[0] + " - " + loKeyRange[1]);

// Get loop start range (depends on SampleStart and LoopEnd)
var loopRange = s.getRange(Sample.LoopStart);
Console.print("LoopStart valid range: " + loopRange[0] + " - " + loopRange[1]);
```

```json:testMetadata:dynamic-range-query
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

**Cross References:**
- `$API.Sample.get$`
- `$API.Sample.set$`
- `$API.Sample.setFromJSON$`
