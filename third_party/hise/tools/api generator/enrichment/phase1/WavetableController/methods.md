# WavetableController -- Method Analysis

## getResynthesisOptions

**Signature:** `JSON getResynthesisOptions()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a DynamicObject with string property names via toVar().
**Minimal Example:** `var opts = {obj}.getResynthesisOptions();`

**Description:**
Returns the current resynthesis options as a JSON object. The returned object contains all parameters that control how audio data is analyzed and converted into wavetable cycles. Modify individual properties on the returned object and pass it back to `setResynthesisOptions` to update settings. See `setResynthesisOptions` for the full property reference.

**Parameters:**

(none)

**Cross References:**
- `$API.WavetableController.setResynthesisOptions$`

## loadData

**Signature:** `undefined loadData(var bufferOrFile, var sampleRate, var loopRange)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Buffer operations, potential file I/O when loading from ScriptFile.
**Minimal Example:** `{obj}.loadData(audioBuffer, 44100.0, [0, 44100]);`

**Description:**
Loads audio data into the wavetable synth for resynthesis. Accepts three input types for the first parameter: a ScriptFile object (loads from an audio file reference), an Array of Buffer objects (multi-channel loading), or a single Buffer (mono loading). When loading from a ScriptFile, the `sampleRate` and `loopRange` parameters are ignored -- the file reference carries its own metadata. Call `resynthesise` after loading data to trigger wavetable generation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| bufferOrFile | NotUndefined | no | Audio source: ScriptFile, Array of Buffers, or single Buffer | See description for accepted types |
| sampleRate | Number | no | Sample rate of the audio data in Hz | Ignored when loading from ScriptFile |
| loopRange | Array | no | Loop range as [startSample, endSample] | Must have exactly 2 integer elements |

**Pitfalls:**
- When loading from a ScriptFile, the `sampleRate` and `loopRange` parameters are silently ignored. The file reference provides its own metadata.
- [BUG] The `loopRange` parameter is silently ignored if it is not an array with exactly 2 elements. No error or warning is produced for invalid formats.
- [BUG] When loading from an array of Buffers, if any element in the array is not a valid Buffer, the corresponding channel pointer is left uninitialized in the stack-allocated array. This can cause undefined behavior when the AudioSampleBuffer is constructed from the pointer array.

**Cross References:**
- `$API.WavetableController.resynthesise$`
- `$API.WavetableController.setResynthesisOptions$`

## resynthesise

**Signature:** `undefined resynthesise()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Triggers heavy FFT-based resynthesis and SiTraNo decomposition on the sample loading thread.
**Minimal Example:** `{obj}.resynthesise();`

**Description:**
Triggers resynthesis of the currently loaded audio data using the current resynthesis options. This delegates to `reloadWavetable()` which performs FFT analysis, optional Loris decomposition, and SiTraNo noise separation on the sample loading thread. Call `loadData` first to provide audio source material, and `setResynthesisOptions` to configure analysis parameters. If a resynthesis cache is enabled, cached results are used when available.

**Parameters:**

(none)

**Cross References:**
- `$API.WavetableController.loadData$`
- `$API.WavetableController.setResynthesisOptions$`
- `$API.WavetableController.setEnableResynthesisCache$`
- `$API.WavetableController.setErrorHandler$`
- `$API.WavetableController.setPostFXProcessors$`

## saveAsAudioFile

**Signature:** `undefined saveAsAudioFile(var outputFile)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** File I/O operations (delete, create, write WAV).
**Minimal Example:** `{obj}.saveAsAudioFile(myFile);`

**Description:**
Saves the currently loaded wavetable as a WAV audio file at 48000 Hz, 24-bit. The output includes loop metadata (`Loop0Start=0`, `Loop0End=tableSize-1`, `NumSampleLoops=1`) so the file can be reimported with correct loop points. The file parameter is resolved via `FileSystem.getFileFromVar`, accepting both ScriptFile objects and string paths.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| outputFile | ScriptObject | no | A ScriptFile object or file reference for the output WAV | -- |

**Pitfalls:**
- [BUG] Does not report an error when the wavetable synth reference is invalid. All other methods in this class call `reportScriptError("No wavetable synth")` on failure, but this method silently returns.

**Cross References:**
- `$API.WavetableController.saveAsHwt$`
- `$API.WavetableController.resynthesise$`

## saveAsHwt

**Signature:** `undefined saveAsHwt(var outputFile)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** File I/O operations (delete, create, write binary stream).
**Minimal Example:** `{obj}.saveAsHwt(myFile);`

**Description:**
Saves the currently loaded wavetable data as an HWT file (HISE Wavetable binary format). The HWT format stores the wavetable as a binary ValueTree that can be loaded back into a WavetableSynth. The parameter must be a ScriptFile object -- string paths are not accepted (unlike `saveAsAudioFile` which uses `getFileFromVar`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| outputFile | ScriptObject | no | A ScriptFile object for the output path | Must be a ScriptFile instance |

**Pitfalls:**
- [BUG] Silently does nothing if no wavetable data is currently loaded in the synth. No error message is produced -- check that `resynthesise` or another loading mechanism has completed before calling.
- [BUG] Silently does nothing if the parameter is not a ScriptFile object. Passing a string path or other object type has no effect and produces no error.

**Cross References:**
- `$API.WavetableController.saveAsAudioFile$`
- `$API.WavetableController.resynthesise$`

## setEnableResynthesisCache

**Signature:** `undefined setEnableResynthesisCache(var cacheDirectory, var clearCache)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** File system operations (directory access, optional deletion of .tmp files).
**Minimal Example:** `{obj}.setEnableResynthesisCache(cacheFolder, false);`

**Description:**
Enables caching of resynthesis results in the specified directory. Cached results are stored as `.tmp` files with names derived from a hash of the source filename and the resynthesis options JSON, so cache entries are automatically invalidated when either changes. Pass `true` for `clearCache` to delete all existing `.tmp` files in the cache folder before setting the new cache location.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| cacheDirectory | ScriptObject | no | A ScriptFile object pointing to the cache folder | Must be a valid directory path |
| clearCache | Integer | no | Whether to clear existing cache files before setting the new path | true/false |

**Cross References:**
- `$API.WavetableController.resynthesise$`
- `$API.WavetableController.setResynthesisOptions$`

## setErrorHandler

**Signature:** `undefined setErrorHandler(var errorCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a WeakCallbackHolder object with heap allocation.
**Minimal Example:** `{obj}.setErrorHandler(onResynthesisError);`

**Description:**
Sets a callback function that receives error messages during resynthesis. The callback is invoked with a single string argument describing the error. Errors from the resynthesis process (FFT failures, Loris issues, etc.) are routed through the `WeakErrorHandler` interface to this callback. Unlike most methods in this class, `setErrorHandler` does not validate the wavetable synth reference -- it only checks that the argument is a JavaScript function.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| errorCallback | Function | no | Error handler callback receiving a string message | Must be a JavaScript function |

**Callback Signature:** errorCallback(errorMessage: String)

**Cross References:**
- `$API.WavetableController.resynthesise$`
- `$API.WavetableController.setResynthesisOptions$`

**Example:**

```javascript:wavetable-error-handler
// Title: Handle resynthesis errors with a custom callback
const var wtc = Synth.getWavetableController("WavetableSynth1");

inline function onResynthesisError(message)
{
    Console.print("Resynthesis error: " + message);
}

wtc.setErrorHandler(onResynthesisError);
```

```json:testMetadata:wavetable-error-handler
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module in the signal chain"
}
```

## setPostFXProcessors

**Signature:** `undefined setPostFXProcessors(var postFXData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires CriticalSection (postFXLock), triggers killVoicesAndCall for safe re-rendering on the sample loading thread.
**Minimal Example:** `{obj}.setPostFXProcessors([{"Type": "Tanh"}, {"Type": "Normalise"}]);`

**Description:**
Sets the post-processing effects chain applied to the wavetable after resynthesis. The input is an array of JSON objects, each defining one processor with a type and optional parameter range. After setting the processors, the wavetable is automatically re-rendered: original resynthesised data is restored, all PostFX are applied to each cycle, and mipmaps are rebuilt. The normalized cycle position (0.0 to 1.0) drives each processor's parameter, allowing effects to vary across the wavetable. An optional Table connection provides additional waveshaping of this index for the "Custom" type.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| postFXData | Array | no | Array of PostFX processor configuration objects | Each element must be a JSON object with a Type property |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Type | String | Processor type name. See Value Descriptions |
| TableProcessor | String | Module ID of a processor providing a Table for "Custom" type waveshaping |
| TableIndex | int | Index of the table within the TableProcessor |
| min | double | Parameter range minimum (maps normalized 0-1 cycle position to this range) |
| max | double | Parameter range maximum |
| skew | double | Parameter range skew factor |
| step | double | Parameter range step size |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Identity" | No-op passthrough, no effect applied to cycles |
| "Custom" | Custom waveshaping using a connected Table lookup. Requires TableProcessor and TableIndex |
| "Sin" | Sine-based waveshaping distortion |
| "Warp" | Time-domain cycle warping |
| "FM1" | FM modulation at 1x carrier frequency |
| "FM2" | FM modulation at 2x carrier frequency |
| "FM3" | FM modulation at 3x carrier frequency |
| "FM4" | FM modulation at 4x carrier frequency |
| "Sync" | Hard sync oscillator effect |
| "Root" | Additive sine wave injection |
| "Clip" | Hard clipping distortion |
| "Tanh" | Soft saturation via hyperbolic tangent |
| "Bitcrush" | Bit depth reduction |
| "SampleAndHold" | Sample-and-hold downsampling |
| "Fold" | Wavefolding distortion |
| "Normalise" | Gain normalization across all cycles |
| "Phase" | Phase rotation of cycles |

**Cross References:**
- `$API.WavetableController.resynthesise$`

**Example:**

```javascript:wavetable-postfx-chain
// Title: Apply a post-FX chain with saturation and normalization
const var wtc = Synth.getWavetableController("WavetableSynth1");

wtc.setPostFXProcessors([
    {"Type": "Tanh", "min": 0.0, "max": 2.0},
    {"Type": "Normalise"}
]);
```

```json:testMetadata:wavetable-postfx-chain
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module with loaded wavetable data"
}
```

## setResynthesisOptions

**Signature:** `undefined setResynthesisOptions(var optionData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Parses JSON object, constructs ResynthesisOptions struct with string operations and heap allocations.
**Minimal Example:** `{obj}.setResynthesisOptions({"PhaseMode": "DynamicPhase", "NumCycles": 64});`

**Description:**
Sets the resynthesis options from a JSON object. Any properties not specified in the object retain their compiled defaults. The options control how audio data is analyzed and converted into wavetable cycles during the next `resynthesise` call. Also attaches this controller instance as the error handler for the resynthesis process, so errors are routed through the callback set via `setErrorHandler`. Options are applied with `dontSendNotification` -- they do not trigger an automatic resynthesis.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| optionData | JSON | no | Resynthesis configuration object | See Callback Properties for valid keys |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| PhaseMode | String | Phase alignment mode (default "StaticPhase"). See Value Descriptions |
| MipMapSize | int | Semitones per mipmap band (default 12) |
| CycleMultiplier | int | Multiplier for cycle length detection (default 4) |
| UseTransientMode | bool | Enable transient detection during resynthesis (default true) |
| NumCycles | int | Fixed cycle count, -1 for auto-detect (default -1). Clamped to max 512, rounded to next power of two |
| ForceResynthesis | bool | Force resynthesis even when a cached result exists (default false) |
| UseLoris | bool | Use Loris library for resynthesis. Only available when HISE is built with HISE_INCLUDE_LORIS (default true when available, false otherwise) |
| ReverseOrder | bool | Reverse the order of extracted cycles (default false) |
| RemoveNoise | bool | Enable noise removal via SiTraNo decomposition (default true) |
| DenoiseSettings | Object | SiTraNo denoising sub-object with properties: slowFFTOrder (int, default 13), fastFFTOrder (int, default 9), freqResolution (double, default 500.0), timeResolution (double, default 0.2), calculateTransients (bool, default true) |
| RootNote | int | Root note for pitch detection, -1 for auto-detect (default -1) |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Resample" | Simple resampling without phase alignment between cycles |
| "ZeroPhase" | Aligns all cycles to zero phase for consistent playback |
| "StaticPhase" | Preserves the original phase relationship across cycles (default) |
| "DynamicPhase" | Tracks phase independently per cycle, preserving natural phase evolution |

**Pitfalls:**
- [BUG] When `RemoveNoise` is omitted from the options JSON, it defaults to the current value of `ReverseOrder` instead of `true` due to a copy-paste error in the C++ deserialization code (`removeNoise = o.getProperty("RemoveNoise", reverseOrder)`). Always set `RemoveNoise` explicitly to avoid inheriting an unintended value.

**Cross References:**
- `$API.WavetableController.getResynthesisOptions$`
- `$API.WavetableController.resynthesise$`
- `$API.WavetableController.setErrorHandler$`

**Example:**

```javascript:configure-resynthesis
// Title: Configure resynthesis options for high-quality wavetable generation
const var wtc = Synth.getWavetableController("WavetableSynth1");

var opts = wtc.getResynthesisOptions();
opts.PhaseMode = "DynamicPhase";
opts.NumCycles = 128;
opts.RemoveNoise = true;
opts.RootNote = 60;
wtc.setResynthesisOptions(opts);
```

```json:testMetadata:configure-resynthesis
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module in the signal chain"
}
```
