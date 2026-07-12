# Buffer -- Methods

## applyMedianFilter

**Signature:** `Buffer applyMedianFilter(Integer windowSize)`
**Return Type:** `Buffer`
**Call Scope:** unsafe
**Minimal Example:** `var filtered = {obj}.applyMedianFilter(7);`

**Description:**
Applies a median filter to the buffer and returns a new buffer containing the filtered samples. The source buffer is not modified.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| windowSize | Integer | no | Median filter window length in samples. | Should be > 0. |

**Pitfalls:**
- [BUG] Calling this method without `windowSize` returns `undefined` instead of reporting an error.

**Cross References:**
- `$API.Buffer.decompose$`
- `$API.Buffer.trim$`
- `$API.Buffer.getRMSLevel$`

**Example:**
```javascript:median-filter-buffer
// Title: Smooth short spikes with a median filter
const var b = Buffer.create(8);
b[0] = 0.0; b[1] = 0.0; b[2] = 1.0; b[3] = 0.0;
b[4] = 0.0; b[5] = 1.0; b[6] = 0.0; b[7] = 0.0;

const var filtered = b.applyMedianFilter(3);
```
```json:testMetadata:median-filter-buffer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "filtered.length", "value": 8},
    {"type": "REPL", "expression": "filtered[2]", "value": 0},
    {"type": "REPL", "expression": "b[2]", "value": 1}
  ]
}
```

## decompose

**Signature:** `Array decompose(Double sampleRate, JSON configData)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var parts = {obj}.decompose(44100.0, {"CalculateTransients": true});`

**Description:**
Runs sinusoidal-transient-noise decomposition and returns an array of output buffers. The returned structure is `[sinusoidal, noise, transient?, noiseGrains]`, where `transient` is only present when transient calculation is enabled, and `noiseGrains` is always the final element as an array of Buffer objects.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sampleRate | Double | no | Sample rate used by the decomposition algorithm. | Defaults to `44100.0` when omitted. |
| configData | JSON | no | Optional configuration object for FFT orders, resolutions, and transient controls. | Property names are case-sensitive and must match C++ keys. |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| SlowFFTOrder | Integer | Slow-stage FFT order, clamped to 5..15. |
| FastFFTOrder | Integer | Fast-stage FFT order, clamped to 5..15. |
| FreqResolution | Double | Frequency resolution parameter for the analysis pass. |
| TimeResolution | Double | Time resolution parameter for the analysis pass. |
| CalculateTransients | Integer | Boolean flag (0/1) that enables transient extraction. |
| SlowTransientTreshold | Array | Two threshold values for the slow stage, clamped to 0..1 and sorted descending. |
| FastTransientTreshold | Array | Two threshold values for the fast stage, clamped to 0..1 and sorted descending. |

**Pitfalls:**
- [BUG] Key names use `...Treshold` (without the second `h`). Using `...Threshold` is silently ignored.
- [BUG] `FastTransientTreshold` parsing checks `SlowTransientTreshold.size()` by mistake, so fast thresholds are ignored unless the slow-threshold array also has size 2.

**Cross References:**
- `$API.Buffer.applyMedianFilter$`
- `$API.Buffer.resample$`
- `$API.Buffer.detectPitch$`

**Example:**
```javascript:decompose-with-config
// Title: Split a buffer into sinusoidal and noise components
const var b = Buffer.create(128);
const var parts = b.decompose(44100.0, {
    "CalculateTransients": false,
    "SlowFFTOrder": 11,
    "FastFFTOrder": 9
});
```
```json:testMetadata:decompose-with-config
{
  "testable": false,
  "skipReason": "Current HISE runtime exits when calling Buffer.decompose in validation harness; keep as documentation-only example until engine issue is resolved."
}
```

## detectPitch

**Signature:** `Double detectPitch(Double sampleRate, Integer startSample, Integer numSamples)`
**Return Type:** `Double`
**Call Scope:** unsafe
**Minimal Example:** `var hz = {obj}.detectPitch(44100.0, 0, {obj}.length);`

**Description:**
Estimates the fundamental frequency of the selected sample region and returns the detected pitch in Hz. If `numSamples` is omitted, it analyzes up to the end of the buffer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sampleRate | Double | no | Sample rate used for frequency scaling. | Required. |
| startSample | Integer | no | Start index inside the buffer. | Defaults to `0` when omitted. |
| numSamples | Integer | no | Number of samples to analyze. | Defaults to remaining buffer length. |

**Pitfalls:**
- This method only exists when HISE is built with `HISE_INCLUDE_PITCH_DETECTION`; in builds without it, `Buffer.detectPitch` is not registered.
- [BUG] Negative `startSample` is not clamped and can produce out-of-bounds reads.

**Cross References:**
- `$API.Buffer.getMagnitude$`
- `$API.Buffer.getRMSLevel$`
- `$API.Buffer.decompose$`

## resample

**Signature:** `Buffer resample(Double ratio, String interpolationType, Integer wrapAround)`
**Return Type:** `Buffer`
**Call Scope:** unsafe
**Minimal Example:** `var out = {obj}.resample(0.5, "Lagrange", false);`

**Description:**
Creates and returns a new buffer resampled by `ratio` using the selected interpolation algorithm. Output length is `round(inputLength / ratio)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| ratio | Double | no | Resampling ratio passed to JUCE interpolators. | Clamped to `0.01..1000.0`. |
| interpolationType | String | no | Interpolator quality mode. | One of `WindowedSinc`, `Lagrange`, `CatmullRom`, `Linear`, `ZeroOrderHold`. |
| wrapAround | Integer | no | Boolean flag (0/1) for boundary handling. | Defaults to `0` (false). |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `"WindowedSinc"` | Highest quality interpolation, highest CPU cost. |
| `"Lagrange"` | Polynomial interpolation with medium quality and cost. |
| `"CatmullRom"` | Cubic interpolation profile between Lagrange and Linear in quality. |
| `"Linear"` | Low-cost linear interpolation (default). |
| `"ZeroOrderHold"` | Sample-and-hold interpolation with lowest quality and lowest CPU cost. |

**Pitfalls:**
- None.

**Cross References:**
- `$API.Buffer.getSlice$`
- `$API.Buffer.trim$`

**Example:**
```javascript:buffer-resample-linear
// Title: Downsample a buffer with linear interpolation
const var b = Buffer.create(16);
b[4] = 1.0;
const var half = b.resample(2.0, "Linear", false);
```
```json:testMetadata:buffer-resample-linear
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "half.length", "value": 8},
    {"type": "REPL", "expression": "half[2]", "value": 1}
  ]
}
```

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Minimal Example:** `var encoded = {obj}.toBase64();`

**Description:**
Serializes the raw float data and returns a Base64 string prefixed with `Buffer`.

**Parameters:**

None.

**Pitfalls:**
- `fromBase64` rejects payloads larger than 44100 samples, so very large serialized buffers cannot be restored with that method.

**Cross References:**
- `$API.Buffer.fromBase64$`

## toCharString

**Signature:** `String toCharString(Integer numChars, Array range)`
**Return Type:** `String`
**Call Scope:** unsafe
**Minimal Example:** `var compact = {obj}.toCharString(64, [0.0, 1.0]);`

**Description:**
Encodes buffer magnitudes into a compact two-character-per-bin string. Each output pair represents one analysis segment of the source buffer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numChars | Integer | no | Target number of analysis bins. | Defaults to buffer length. |
| range | Array | no | Two-value `[min, max]` clamp range before encoding. | Defaults to `[0.0, 1.0]`. |

**Pitfalls:**
- [BUG] If `numChars` is larger than buffer length, internal `samplesPerChar` becomes 0 and the loop step never advances.

**Cross References:**
- `$API.Buffer.getPeakRange$`
- `$API.Buffer.getMagnitude$`

**Example:**
```javascript:buffer-char-encoding
// Title: Create a compact display string from sample data
const var b = Buffer.create(32);
const var encoded = b.toCharString(16, [-1.0, 1.0]);
```
```json:testMetadata:buffer-char-encoding
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "encoded.length",
    "value": 32
  }
}
```

## trim

**Signature:** `Buffer trim(Integer trimFromStart, Integer trimFromEnd)`
**Return Type:** `Buffer`
**Call Scope:** unsafe
**Minimal Example:** `var shorter = {obj}.trim(128, 128);`

**Description:**
Returns a new copied buffer with samples removed from the start and end. The source buffer is unchanged.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| trimFromStart | Integer | no | Number of samples to remove from the start. | Clamped to `0..length-1`. |
| trimFromEnd | Integer | no | Number of samples to remove from the end after start trimming. | Clamped to the remaining range. |

**Pitfalls:**
- None.

**Cross References:**
- `$API.Buffer.getSlice$`
- `$API.Buffer.resample$`

## getRMSLevel

**Signature:** `Double getRMSLevel(Integer startSample, Integer numSamples)`
**Return Type:** `Double`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive O(n) analysis over the selected range.
**Minimal Example:** `var rms = {obj}.getRMSLevel(0, {obj}.length);`

**Description:**
Returns the RMS (root mean square) level of the selected sample region.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startSample | Integer | no | Start index for the RMS window. | Should be >= 0. |
| numSamples | Integer | no | Number of samples to include. | Defaults to full remaining length. |

**Pitfalls:**
- [BUG] Negative `startSample` is not clamped and can cause out-of-bounds reads.

**Cross References:**
- `$API.Buffer.getMagnitude$`
- `$API.Buffer.getPeakRange$`

## getSlice

**Signature:** `Buffer getSlice(Integer offsetInBuffer, Integer numSamples)`
**Return Type:** `Buffer`
**Call Scope:** unsafe
**Minimal Example:** `var slice = {obj}.getSlice(256, 512);`

**Description:**
Returns a new Buffer object that references a subrange of the original buffer. The returned slice shares memory with the source buffer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| offsetInBuffer | Integer | no | Start offset of the slice. | Defaults to `0`. |
| numSamples | Integer | no | Slice length. | Defaults to remaining length after offset. |

**Pitfalls:**
- The slice is a reference view, not a copy. Writing to the slice also changes the source buffer region.
- [BUG] Negative offsets are not clamped and can create invalid pointer offsets.

**Cross References:**
- `$API.Buffer.trim$`
- `$API.Buffer.resample$`

**Example:**
```javascript:buffer-slice-reference
// Title: Slice shares storage with the source buffer
const var source = Buffer.create(6);
source[0] = 1.0; source[1] = 2.0; source[2] = 3.0;
source[3] = 4.0; source[4] = 5.0; source[5] = 6.0;

const var slice = source.getSlice(2, 2);
slice[0] = 99.0;
```
```json:testMetadata:buffer-slice-reference
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "slice.length", "value": 2},
    {"type": "REPL", "expression": "source[2]", "value": 99}
  ]
}
```

## indexOfPeak

**Signature:** `Integer indexOfPeak(Integer startSample, Integer numSamples)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Linear scan over the selected range.
**Minimal Example:** `var peakIndex = {obj}.indexOfPeak(0, {obj}.length);`

**Description:**
Finds the index of the sample with the highest absolute value in the selected range.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startSample | Integer | no | Start index of the scan. | Defaults to `0`. |
| numSamples | Integer | no | Number of samples to scan. | Defaults to full remaining length. |

**Pitfalls:**
- [BUG] Negative `startSample` is not clamped and can cause out-of-bounds reads.

**Cross References:**
- `$API.Buffer.getMagnitude$`
- `$API.Buffer.getNextZeroCrossing$`

## normalise

**Signature:** `undefined normalise(Double gainInDecibels)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** In-place O(n) normalization over the full buffer.
**Minimal Example:** `{obj}.normalise(-6.0);`

**Description:**
Scales the buffer in place so its peak magnitude becomes the target level.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| gainInDecibels | Double | no | Target peak level in dB. | Defaults to 0 dB when omitted. |

**Pitfalls:**
- [BUG] The implementation ignores `gainInDecibels` and always normalizes to 0 dB peak.

**Cross References:**
- `$API.Buffer.getMagnitude$`
- `$API.Buffer.getRMSLevel$`

## fromBase64

**Signature:** `Integer fromBase64(String b64String)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Minimal Example:** `var ok = {obj}.fromBase64(serializedBuffer);`

**Description:**
Decodes a Base64 payload created by `Buffer.toBase64()`, resizes this buffer to the decoded sample count, and copies the decoded float data into it. Returns `1` on success and `0` when the input has the wrong prefix or cannot be decoded.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64String | String | no | Base64 string with the `Buffer` prefix. | Maximum decoded size is 44100 samples. |

**Pitfalls:**
- The method requires the literal `Buffer` prefix. Plain Base64 data without this prefix returns `0` silently.

**Cross References:**
- `$API.Buffer.toBase64$`

**Example:**
```javascript:buffer-base64-roundtrip
// Title: Restore a serialized buffer payload
const var source = Buffer.create(4);
source[0] = 0.1; source[1] = 0.2; source[2] = 0.3; source[3] = 0.4;
const var encoded = source.toBase64();

const var target = Buffer.create(1);
const var ok = target.fromBase64(encoded);
```
```json:testMetadata:buffer-base64-roundtrip
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "ok", "value": 1},
    {"type": "REPL", "expression": "target.length", "value": 4}
  ]
}
```

## getMagnitude

**Signature:** `Double getMagnitude(Integer startSample, Integer numSamples)`
**Return Type:** `Double`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive O(n) scan over the selected sample range.
**Minimal Example:** `var peak = {obj}.getMagnitude(0, {obj}.length);`

**Description:**
Returns the absolute peak magnitude in the selected region. If the buffer is empty, it returns `0.0`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startSample | Integer | no | Start index for the range scan. | Clamped to valid range. |
| numSamples | Integer | no | Number of samples to include. | Clamped to available range. |

**Pitfalls:**
- None.

**Cross References:**
- `$API.Buffer.getRMSLevel$`
- `$API.Buffer.getPeakRange$`
- `$API.Buffer.indexOfPeak$`
- `$API.Buffer.detectPitch$`

## getNextZeroCrossing

**Signature:** `Integer getNextZeroCrossing(Integer index)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive linear scan from `index` to the end of the buffer.
**Minimal Example:** `var cross = {obj}.getNextZeroCrossing(0);`

**Description:**
Searches forward for the next negative-to-positive sign transition and returns its index. Returns `-1` if no crossing is found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Start position for the forward scan. | Should be >= 0 and `< length - 1`. |

**Pitfalls:**
- [BUG] Negative indices are not clamped and can cause out-of-bounds reads.

**Cross References:**
- `$API.Buffer.indexOfPeak$`

## getPeakRange

**Signature:** `Array getPeakRange(Integer startSample, Integer numSamples)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var range = {obj}.getPeakRange(0, {obj}.length);`

**Description:**
Returns a two-element array `[minValue, maxValue]` for the selected sample region.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startSample | Integer | no | Start index for the range analysis. | Should be >= 0. |
| numSamples | Integer | no | Number of samples to scan. | Defaults to full remaining length. |

**Pitfalls:**
- [BUG] Negative `startSample` is not clamped and can cause out-of-bounds reads.

**Cross References:**
- `$API.Buffer.getMagnitude$`
- `$API.Buffer.getRMSLevel$`
- `$API.Buffer.toCharString$`
