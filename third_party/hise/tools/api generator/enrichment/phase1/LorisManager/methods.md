## analyse

**Signature:** `Integer analyse(ScriptObject file, Double rootFrequency)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Heavy FFT-based spectral analysis with heap allocations and DLL/library calls.
**Minimal Example:** `var ok = {obj}.analyse(audioFile, 440.0);`

**Description:**
Analyses an audio file using the Loris partial tracking algorithm. The root frequency is the estimated fundamental frequency of the sound in Hz and guides the harmonic partial tracking. Returns true on success, false if the file parameter is not a valid ScriptFile object. This must be the first operation in any Loris workflow -- all other methods (`synthesise`, `process`, `processCustom`, `createEnvelopes`, `createEnvelopePaths`, `createSnapshot`) require a prior analysis of the same file.

When caching is enabled (the default), re-analysing the same file reuses the cached partial list. Disable caching with `set("enablecache", false)` to force re-analysis.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | ScriptObject | no | The audio file to analyse. Must be a ScriptFile object obtained from FileSystem. | ScriptFile only |
| rootFrequency | Double | no | Estimated fundamental frequency of the sound in Hz. | > 0 |

**Pitfalls:**
- Passing a file path string or any non-ScriptFile object silently returns false without an error message. Use `FileSystem.fromAbsolutePath()` or `FileSystem.browse()` to obtain a ScriptFile object.

**Cross References:**
- `$API.LorisManager.set$`
- `$API.LorisManager.synthesise$`
- `$API.LorisManager.process$`
- `$API.LorisManager.processCustom$`

## createEnvelopePaths

**Signature:** `Array createEnvelopePaths(ScriptObject file, String parameter, Integer harmonicIndex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Calls createEnvelopes internally, then creates Path objects with heap allocations.
**Minimal Example:** `var paths = {obj}.createEnvelopePaths(audioFile, "gain", 0);`

**Description:**
Creates an array of Path objects representing the envelope of the specified parameter for a given harmonic index, one Path per audio channel. Internally calls `createEnvelopes` to get the raw buffer data, then converts each buffer to a downsampled Path suitable for display. The Path is clipped to the valid range for the parameter and downsampled to approximately 200 display points.

Pass `0` for harmonicIndex to get the envelope for the fundamental. The file must have been previously analysed with `analyse()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | ScriptObject | no | The previously analysed audio file. Must be a ScriptFile object. | ScriptFile only |
| parameter | String | no | The partial parameter to create paths for. | See Value Descriptions |
| harmonicIndex | Integer | no | The harmonic index (0 = fundamental). | >= 0 |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "rootFrequency" | F0 estimate relative to root frequency, centered on 1.0. Range derived from freqdrift setting. |
| "frequency" | Partial frequency in Hz, centered on 1.0. Range derived from freqdrift setting. |
| "phase" | Phase in radians. Range: -PI to PI. |
| "gain" | Amplitude. Range: 0.0 to 1.0. |
| "bandwidth" | Noisiness. Range: 0.0 (pure sine) to 1.0 (full noise). |

**Pitfalls:**
- Passing a non-ScriptFile object silently returns an empty array without an error message.

**Cross References:**
- `$API.LorisManager.createEnvelopes$`
- `$API.LorisManager.analyse$`

## createEnvelopes

**Signature:** `Array createEnvelopes(ScriptObject file, String parameter, Integer harmonicIndex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates heap buffers for envelope data and creates VariantBuffer objects per channel.
**Minimal Example:** `var envelopes = {obj}.createEnvelopes(audioFile, "gain", 0);`

**Description:**
Creates an array of audio-rate Buffer objects representing the envelope of the specified parameter for a given harmonic index, one Buffer per audio channel. Each buffer contains the envelope sampled at the audio file's sample rate. The file must have been previously analysed with `analyse()`.

Pass `0` for harmonicIndex to get the envelope for the fundamental. The `rootFrequency` parameter uses the F0 estimate internally rather than individual partial labels. All other parameters require Loris's internal `prepareToMorph()` step (channelization, collation, sifting, distillation, sorting) which is handled automatically.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | ScriptObject | no | The previously analysed audio file. Must be a ScriptFile object. | ScriptFile only |
| parameter | String | no | The partial parameter to extract envelopes for. | See Value Descriptions |
| harmonicIndex | Integer | no | The harmonic index (0 = fundamental). | >= 0 |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "rootFrequency" | F0 estimate relative to root frequency, centered on 1.0. Range derived from freqdrift setting. |
| "frequency" | Partial frequency in Hz, centered on 1.0. Range derived from freqdrift setting. |
| "phase" | Phase in radians. Range: -PI to PI. |
| "gain" | Amplitude. Range: 0.0 to 1.0. |
| "bandwidth" | Noisiness. Range: 0.0 (pure sine) to 1.0 (full noise). |

**Pitfalls:**
- Passing a non-ScriptFile object silently returns an empty array without an error message.

**Cross References:**
- `$API.LorisManager.createEnvelopePaths$`
- `$API.LorisManager.createSnapshot$`
- `$API.LorisManager.analyse$`

## createSnapshot

**Signature:** `Array createSnapshot(ScriptObject file, String parameter, Double time)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates heap buffer (NUM_MAX_CHANNELS * 512 doubles), DLL/library call.
**Minimal Example:** `var snapshot = {obj}.createSnapshot(audioFile, "gain", 0.5);`

**Description:**
Creates a snapshot of a partial parameter at a specific time point, returning the value for each harmonic across all channels. Returns a nested array structure: the outer array has one entry per audio channel, and each inner array contains the parameter value for each harmonic at the specified time.

The time value is interpreted according to the current `timedomain` setting (default: seconds). The file must have been previously analysed with `analyse()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | ScriptObject | no | The previously analysed audio file. Must be a ScriptFile object. | ScriptFile only |
| parameter | String | no | The partial parameter to snapshot. | See Value Descriptions |
| time | Double | no | The time position for the snapshot. | Interpreted per timedomain setting |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "rootFrequency" | F0 estimate relative to root frequency, centered on 1.0. |
| "frequency" | Partial frequency in Hz. |
| "phase" | Phase in radians. |
| "gain" | Amplitude (0.0 to 1.0). |
| "bandwidth" | Noisiness (0.0 = pure sine, 1.0 = full noise). |

**Pitfalls:**
- Passing a non-ScriptFile object silently returns an empty array without an error message.
- The internal buffer is hardcoded to 512 harmonics per channel. Sounds with more than 512 tracked partials will have their higher harmonics silently truncated.

**Cross References:**
- `$API.LorisManager.createEnvelopes$`
- `$API.LorisManager.createEnvelopePaths$`
- `$API.LorisManager.analyse$`
- `$API.LorisManager.set$`

## get

**Signature:** `Double get(String optionId)`
**Return Type:** `Double`
**Call Scope:** unsafe
**Call Scope Note:** String operations and DLL/library function call.
**Minimal Example:** `var val = {obj}.get("windowwidth");`

**Description:**
Returns the current value of a Loris analysis option as a number. All options return numeric values, including boolean options (0.0 or 1.0 for `enablecache`) and the `timedomain` option (returns an internal numeric code, not the string name).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| optionId | String | no | The option identifier to query. | See Value Descriptions |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "timedomain" | Current time domain mode (returned as numeric code). |
| "enablecache" | Whether partial caching is enabled (0.0 or 1.0). |
| "windowwidth" | Window width scale factor (0.125 to 4.0). |
| "freqfloor" | Lowest frequency considered harmonic content in Hz. |
| "ampfloor" | Lowest amplitude above noise floor in dB. |
| "sidelobes" | Side lobe gain of analysis window in dB. |
| "freqdrift" | Maximum frequency drift tolerance in cents. |
| "hoptime" | Time between analysis windows in seconds. |
| "croptime" | Crop time parameter in seconds. |
| "bwregionwidth" | Bandwidth region width. |

**Cross References:**
- `$API.LorisManager.set$`

## process

**Signature:** `undefined process(ScriptObject file, String command, JSON data)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** JSON serialization/parsing, DLL/library call, heap allocations during partial list manipulation.
**Minimal Example:** `{obj}.process(audioFile, "shiftPitch", {"offset": 100});`

**Description:**
Processes the analysed partial list using a predefined command. The data parameter is a JSON object or array whose structure depends on the command. The file must have been previously analysed with `analyse()`. Processing modifies the cached partial list in place -- use `process(file, "reset", {})` to revert to the original analysis state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | ScriptObject | no | The previously analysed audio file. Must be a ScriptFile object. | ScriptFile only |
| command | String | no | The processing command to execute. | See Value Descriptions |
| data | JSON | no | Command-specific parameters. | Structure varies by command |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "reset" | Resets partials to original analysis state. Data: empty object `{}`. |
| "shiftTime" | Shifts all partial times by an offset. Data: `{"offset": number}` in the current time domain. |
| "shiftPitch" | Shifts pitch by a constant or envelope. Data: `{"offset": number}` for constant shift in cents, or `[[time, value], ...]` for an envelope. |
| "scaleFrequency" | Scales frequency by an envelope curve. Data: `[[time, value], ...]` array of envelope points. |
| "dilate" | Time-stretches partials using input/target time pairs. Data: `[[inputTimes], [targetTimes]]` where both are arrays of doubles. |
| "applyFilter" | Applies a gain envelope in the frequency domain. Data: `[[freq, value], ...]` array of frequency/gain points. |

**Pitfalls:**
- Passing a non-ScriptFile object silently does nothing without an error message.

**Cross References:**
- `$API.LorisManager.processCustom$`
- `$API.LorisManager.analyse$`
- `$API.LorisManager.synthesise$`

**Example:**
```javascript:loris-process-commands
// Title: Processing partial lists with predefined commands
const var lm = Engine.getLorisManager();
const var audioFile = FileSystem.fromAbsolutePath("C:/audio/test.wav");

lm.analyse(audioFile, 440.0);

// Shift pitch up by 100 cents (one semitone)
lm.process(audioFile, "shiftPitch", {"offset": 100});

// Time-stretch: map times 0.0 and 1.0 to 0.0 and 2.0
lm.process(audioFile, "dilate", [[0.0, 1.0], [0.0, 2.0]]);

// Reset to original analysis state
lm.process(audioFile, "reset", {});
```
```json:testMetadata:loris-process-commands
{
  "testable": false,
  "skipReason": "Requires an audio file on disk for Loris analysis."
}
```

## processCustom

**Signature:** `undefined processCustom(ScriptObject file, Function processCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Iterates over every breakpoint in every partial, calling the script function synchronously for each. Heavy processing with heap allocations.
**Minimal Example:** `{obj}.processCustom(audioFile, onProcessPartial);`

**Description:**
Processes the analysed partial list using a custom callback function. The callback is invoked once for every breakpoint of every partial in the analysed file, receiving a JSON object with the breakpoint's properties. The callback can modify the mutable properties (time, frequency, phase, gain, bandwidth) by changing values on the object directly -- the changes are written back to the partial list after each call.

The file must have been previously analysed with `analyse()`. This method provides fine-grained control over individual partial breakpoints, unlike `process()` which applies predefined bulk operations.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | ScriptObject | no | The previously analysed audio file. Must be a ScriptFile object. | ScriptFile only |
| processCallback | Function | no | A function called for each breakpoint in the partial list. | Must accept 1 argument |

**Callback Signature:** processCallback(data: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| channelIndex | int | Channel in the audio file (read-only). |
| partialIndex | int | Index of the partial (read-only). |
| sampleRate | double | Sample rate of the file (read-only). |
| rootFrequency | double | Root frequency passed to analyse (read-only). |
| time | double | Time of the breakpoint (read-write). |
| frequency | double | Frequency of the partial at the breakpoint in Hz (read-write). |
| phase | double | Phase in radians (read-write). |
| gain | double | Amplitude of the partial (read-write). |
| bandwidth | double | Noisiness, 0.0 = pure sine, 1.0 = full noise (read-write). |

**Pitfalls:**
- Passing a non-ScriptFile object silently does nothing without an error message.
- Modifying read-only properties (channelIndex, partialIndex, sampleRate, rootFrequency) on the callback object has no effect -- changes to these properties are silently ignored because `writeJSON()` only reads back the mutable properties.

**Cross References:**
- `$API.LorisManager.process$`
- `$API.LorisManager.analyse$`
- `$API.LorisManager.synthesise$`

**Example:**
```javascript:loris-custom-processing
// Title: Custom partial processing to filter high-frequency content
const var lm = Engine.getLorisManager();
const var audioFile = FileSystem.fromAbsolutePath("C:/audio/test.wav");

lm.analyse(audioFile, 440.0);

inline function onProcessPartial(data)
{
    // Halve the gain of all partials above 2000 Hz
    if (data.frequency > 2000.0)
        data.gain = data.gain * 0.5;
}

lm.processCustom(audioFile, onProcessPartial);

// Resynthesise the modified partials
var buffers = lm.synthesise(audioFile);
```
```json:testMetadata:loris-custom-processing
{
  "testable": false,
  "skipReason": "Requires an audio file on disk for Loris analysis."
}
```

## set

**Signature:** `undefined set(String optionId, NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** String construction and DLL/library function call.
**Minimal Example:** `{obj}.set("windowwidth", 2.0);`

**Description:**
Sets a Loris analysis option to a new value. The value is converted to a string internally regardless of its type, so both numeric values (e.g., `2.0` for windowwidth) and string values (e.g., `"samples"` for timedomain) are accepted. Options affect subsequent `analyse()` calls. Some options like `windowwidth` are clamped to a valid range automatically.

An invalid option ID produces an error in the Loris library that is reported to the console.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| optionId | String | no | The option identifier to set. | See Value Descriptions |
| newValue | NotUndefined | no | The new value. Converted to string internally. | Type depends on option |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "timedomain" | Time axis domain. Accepts: "seconds" (default), "samples", or "0to1". |
| "enablecache" | Cache analysed partials for reuse. Accepts: true/false or "true"/"false". Default: true. |
| "windowwidth" | Window width scale factor. Numeric, clamped to 0.125 - 4.0. Default: 1.0. |
| "freqfloor" | Lowest frequency considered harmonic content in Hz. Default: 40.0. |
| "ampfloor" | Lowest amplitude above noise floor in dB. Default: 90.0. |
| "sidelobes" | Side lobe gain of analysis window in dB. Default: 90.0. |
| "freqdrift" | Maximum frequency drift tolerance in cents. Default: 50.0. |
| "hoptime" | Time between analysis windows in seconds. Default: 0.0129. |
| "croptime" | Crop time parameter in seconds. Default: 0.0129. |
| "bwregionwidth" | Bandwidth region width. Default: 1.0. |

**Cross References:**
- `$API.LorisManager.get$`
- `$API.LorisManager.analyse$`

## synthesise

**Signature:** `Array synthesise(ScriptObject file)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates heap buffers for audio output, DLL/library call for additive resynthesis.
**Minimal Example:** `var buffers = {obj}.synthesise(audioFile);`

**Description:**
Resynthesises audio from the analysed (and optionally processed) partial list, returning an array of Buffer objects with one buffer per audio channel. Each buffer contains the full resynthesised waveform at the original sample rate. The file must have been previously analysed with `analyse()`.

If the partial list has been modified via `process()` or `processCustom()`, the resynthesis reflects those modifications. To get the original unmodified audio, call `process(file, "reset", {})` before resynthesising.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | ScriptObject | no | The previously analysed audio file. Must be a ScriptFile object. | ScriptFile only |

**Pitfalls:**
- Passing a non-ScriptFile object silently returns an empty array without an error message.

**Cross References:**
- `$API.LorisManager.analyse$`
- `$API.LorisManager.process$`
- `$API.LorisManager.processCustom$`
