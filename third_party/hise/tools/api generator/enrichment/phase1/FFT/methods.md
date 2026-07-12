## getSpectrum2DParameters

**Signature:** `JSON getSpectrum2DParameters()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new DynamicObject and performs string property operations via saveToJSON.
**Minimal Example:** `var params = {obj}.getSpectrum2DParameters();`

**Description:**
Returns the current Spectrum2D display parameters as a JSON object. The returned object contains all configurable spectrogram properties. See `setSpectrum2DParameters()` for the full property schema.

**Parameters:**

(None)

**Cross References:**
- `$API.FFT.setSpectrum2DParameters$`

---

## prepare

**Signature:** `undefined prepare(Integer powerOfTwoSize, Integer maxNumChannels)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates audio buffers, per-channel work buffers, and creates the JUCE FFT engine instance under a write lock.
**Minimal Example:** `{obj}.prepare(1024, 1);`

**Description:**
Allocates all internal buffers required for FFT processing. Must be called before `process()`. The FFT size must be a power of two (e.g., 256, 512, 1024, 2048). The channel count is clamped to [1, 16]. Work buffers are allocated conditionally: magnitude buffers only when a magnitude callback or inverse FFT is enabled, phase buffers only when a phase callback or inverse FFT is enabled, and output buffers only when inverse FFT is enabled. Changing callbacks or enabling inverse FFT after calling `prepare()` triggers automatic reallocation via internal `reinitialise()`. Uses a hardcoded sample rate of 44100 Hz internally.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| powerOfTwoSize | Integer | no | FFT size in samples | Must be a power of two |
| maxNumChannels | Integer | no | Maximum number of audio channels to process | 1-16 (clamped) |

**Cross References:**
- `$API.FFT.process$`
- `$API.FFT.setMagnitudeFunction$`
- `$API.FFT.setEnableInverseFFT$`

**DiagramRef:** fft-processing-pipeline

---

## process

**Signature:** `AudioData process(AudioData dataToProcess)`
**Return Type:** `AudioData`
**Call Scope:** unsafe
**Call Scope Note:** Allocates output buffers, Spectrum2D objects, and spectrum images. Acquires a read lock for the callback processing loop.
**Minimal Example:** `var result = {obj}.process(inputBuffer);`

**Description:**
Runs the FFT processing pipeline on the input audio data. Accepts a single Buffer (mono) or an Array of Buffers (multi-channel).

Two independent code paths can be active simultaneously:

1. **Spectrum2D path** (when `setEnableSpectrum2D(true)`): Generates a spectrogram image from the input buffer before callback processing. The image is stored internally and can be drawn via `Graphics.drawFFTSpectrum()`.

2. **Callback path** (when magnitude/phase functions are set): Processes data in overlapping chunks -- each chunk is windowed, forward-transformed, passed to the magnitude and/or phase callbacks, and optionally inverse-transformed with overlap-add reconstruction.

Returns a Buffer (mono) or Array of Buffers (multi-channel) when inverse FFT is enabled. Returns undefined when inverse FFT is not enabled. Throws an error if `prepare()` was not called, or if neither callbacks nor Spectrum2D mode is active.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataToProcess | AudioData | no | Input audio data | Single Buffer for mono, Array of Buffers for multi-channel |

**Cross References:**
- `$API.FFT.prepare$`
- `$API.FFT.setMagnitudeFunction$`
- `$API.FFT.setPhaseFunction$`
- `$API.FFT.setEnableInverseFFT$`
- `$API.FFT.setEnableSpectrum2D$`

**DiagramRef:** fft-processing-pipeline

**Example:**
```javascript:inverse-fft-pipeline
// Title: Spectral filtering with inverse FFT
const var fft = Engine.createFFT();

inline function onMagnitude(magnitudes, offset)
{
    // Simple low-pass: zero out upper half of spectrum
    local halfSize = magnitudes.length / 2;

    for (i = halfSize; i < magnitudes.length; i++)
        magnitudes[i] = 0.0;
};

fft.setWindowType(fft.Hann);
fft.setOverlap(0.5);
fft.setMagnitudeFunction(onMagnitude, false);
fft.setEnableInverseFFT(true);
fft.prepare(1024, 1);

// Process a mono buffer -- returns a new Buffer with reconstructed signal
// var filtered = fft.process(sourceBuffer);
```
```json:testMetadata:inverse-fft-pipeline
{
  "testable": false,
  "skipReason": "Requires pre-filled audio buffer as process() input"
}
```

---

## setEnableInverseFFT

**Signature:** `undefined setEnableInverseFFT(Integer shouldApplyReverseTransformToInput)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Triggers reinitialise() which reallocates all work buffers under a write lock when the state changes.
**Minimal Example:** `{obj}.setEnableInverseFFT(true);`

**Description:**
Enables or disables inverse FFT reconstruction. When enabled, `process()` reconstructs the time-domain signal from the (possibly modified) magnitude and phase data using overlap-add, and returns the result as a Buffer or Array of Buffers. When disabled, `process()` calls callbacks but does not reconstruct or return audio data. Triggers automatic buffer reallocation only if the state actually changes and `prepare()` has been called previously.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldApplyReverseTransformToInput | Integer | no | Enable (true) or disable (false) inverse FFT | Boolean |

**Cross References:**
- `$API.FFT.process$`
- `$API.FFT.prepare$`

---

## setEnableSpectrum2D

**Signature:** `undefined setEnableSpectrum2D(Integer shouldBeEnabled)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setEnableSpectrum2D(true);`

**Description:**
Enables or disables 2D spectrogram image generation during `process()`. When enabled, `process()` generates a spectrogram image from the input buffer (and optionally from the output buffer when inverse FFT is also active). The generated image can be drawn using `Graphics.drawFFTSpectrum()`. Configure spectrogram appearance with `setSpectrum2DParameters()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeEnabled | Integer | no | Enable (true) or disable (false) spectrum generation | Boolean |

**Cross References:**
- `$API.FFT.process$`
- `$API.FFT.setSpectrum2DParameters$`

---

## setMagnitudeFunction

**Signature:** `undefined setMagnitudeFunction(Function newMagnitudeFunction, Number convertToDecibels)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires a write lock, creates a new WeakCallbackHolder (heap allocation), and triggers reinitialise() which reallocates work buffers.
**Minimal Example:** `{obj}.setMagnitudeFunction(onMagnitude, false);`

**Description:**
Registers a callback function that receives magnitude (amplitude) data for each FFT chunk during `process()`. The callback receives two arguments: the magnitude buffer(s) and the current sample offset in the source data. When `convertToDecibels` is true, magnitude values are scaled to decibels before the callback is invoked. For multi-channel input, the first argument is an Array of Buffers instead of a single Buffer. In the HISE IDE, the callback is validated for realtime safety and an error is reported if it contains non-audio-safe operations. Triggers automatic buffer reallocation if `prepare()` has been called previously.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newMagnitudeFunction | Function | yes | Callback receiving magnitude data per FFT chunk | Must be an inline function with 2 parameters |
| convertToDecibels | Number | yes | Convert magnitudes to dB scale before callback | Boolean (0 or 1) |

**Callback Signature:** newMagnitudeFunction(magnitudes: Buffer, offset: int)

**Cross References:**
- `$API.FFT.setPhaseFunction$`
- `$API.FFT.process$`
- `$API.FFT.prepare$`

**Example:**
```javascript:magnitude-decibel-callback
// Title: Magnitude callback with decibel conversion
const var fft = Engine.createFFT();

inline function onMagnitude(magnitudes, offset)
{
    // magnitudes: frequency amplitudes in dB (when convertToDecibels is true)
    // offset: current chunk position in the source buffer
    Console.print("Chunk at " + offset + ", bins: " + magnitudes.length);
};

// true = magnitudes arrive in dB scale
fft.setMagnitudeFunction(onMagnitude, true);
```
```json:testMetadata:magnitude-decibel-callback
{
  "testable": false,
  "skipReason": "Callback only invoked during process() which requires audio buffer input"
}
```

---

## setOverlap

**Signature:** `undefined setOverlap(Double percentageOfOverlap)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setOverlap(0.5);`

**Description:**
Sets the overlap ratio for chunk-based FFT processing. The value is clamped to [0.0, 0.99]. An overlap of 0.5 means each chunk overlaps 50% with the previous one, resulting in a step size of `fftSize * 0.5` samples. Higher overlap produces smoother spectral analysis at the cost of more processing iterations. The Spectrum2D oversampling factor is derived automatically as the next power of two of `1 / (1 - overlap)` (e.g., overlap 0.5 gives factor 2, overlap 0.75 gives factor 4).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| percentageOfOverlap | Double | no | Overlap ratio between consecutive FFT chunks | 0.0-0.99 (clamped) |

**Cross References:**
- `$API.FFT.process$`
- `$API.FFT.prepare$`
- `$API.FFT.setWindowType$`

---

## setPhaseFunction

**Signature:** `undefined setPhaseFunction(Function newPhaseFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires a write lock, creates a new WeakCallbackHolder (heap allocation), and triggers reinitialise() which reallocates work buffers.
**Minimal Example:** `{obj}.setPhaseFunction(onPhase);`

**Description:**
Registers a callback function that receives phase data for each FFT chunk during `process()`. The callback receives two arguments: the phase buffer(s) containing phase angles for each frequency bin, and the current sample offset in the source data. For multi-channel input, the first argument is an Array of Buffers instead of a single Buffer. Modifications to the phase buffer in the callback are used during inverse FFT reconstruction. In the HISE IDE, the callback is validated for realtime safety. Triggers automatic buffer reallocation if `prepare()` has been called previously.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newPhaseFunction | Function | yes | Callback receiving phase data per FFT chunk | Must be an inline function with 2 parameters |

**Callback Signature:** newPhaseFunction(phases: Buffer, offset: int)

**Cross References:**
- `$API.FFT.setMagnitudeFunction$`
- `$API.FFT.process$`
- `$API.FFT.prepare$`

**Example:**
```javascript:phase-manipulation
// Title: Phase manipulation callback
const var fft = Engine.createFFT();

inline function onPhase(phases, offset)
{
    // phases: Buffer with phase angles for each frequency bin
    // Randomize phases for a spectral diffusion effect
    for (i = 0; i < phases.length; i++)
        phases[i] = Math.random() * 6.283;
};

fft.setPhaseFunction(onPhase);
```
```json:testMetadata:phase-manipulation
{
  "testable": false,
  "skipReason": "Callback only invoked during process() which requires audio buffer input"
}
```

---

## setSpectrum2DParameters

**Signature:** `undefined setSpectrum2DParameters(JSON jsonData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls loadFromJSON which performs string property lookups and value assignments on the parameters object.
**Minimal Example:** `{obj}.setSpectrum2DParameters({"FFTSize": 10, "ColourScheme": 3});`

**Description:**
Configures the Spectrum2D spectrogram renderer from a JSON object. Only the properties included in the object are updated; omitted properties retain their current values. The spectrogram image is generated during `process()` when Spectrum2D is enabled via `setEnableSpectrum2D(true)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | Configuration object with spectrogram properties | See Callback Properties table |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| FFTSize | Integer | Log2 of the FFT size (7=128 to 13=8192 samples) |
| DynamicRange | Integer | Minimum dB value for display (default: 110) |
| Oversampling | Integer | Oversampling factor (default: 4) |
| ColourScheme | Integer | Colour palette: 0=blackWhite, 1=rainbow, 2=violetToOrange, 3=hiseColours, 4=preColours |
| GainFactor | Integer | Gain in units where 1000 = 0.0 dB (default: 1000) |
| ResamplingQuality | String | Image resampling quality: "Low", "Mid", or "High" |
| Gamma | Integer | Gamma correction percentage (0-150, default: 60) |
| Standardize | Integer | Standardize output (boolean, default: false) |
| FrequencyGamma | Integer | Frequency axis gamma (100-200, default: 100) |
| WindowType | Integer | Window type constant (0-6, same as FFT window constants) |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Low" | Fast, lower quality image resampling |
| "Mid" | Balanced quality and performance |
| "High" | Best quality, slower image resampling |

**Cross References:**
- `$API.FFT.getSpectrum2DParameters$`
- `$API.FFT.setEnableSpectrum2D$`

**Example:**
```javascript:spectrum2d-config
// Title: Configuring and verifying spectrogram parameters
const var fft = Engine.createFFT();

fft.setSpectrum2DParameters({
    "FFTSize": 10,
    "Oversampling": 4,
    "ColourScheme": 3,
    "Gamma": 80,
    "DynamicRange": 90,
    "ResamplingQuality": "High"
});

var p = fft.getSpectrum2DParameters();
Console.print(p.Gamma);
```
```json:testMetadata:spectrum2d-config
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["80"]}
}
```

---

## setUseFallbackEngine

**Signature:** `undefined setUseFallbackEngine(Integer shouldUseFallback)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setUseFallbackEngine(true);`

**Description:**
Forces the FFT object to use the JUCE fallback FFT engine instead of platform-optimized implementations (such as vDSP on macOS or IPP on Windows). The fallback engine is required for `dumpSpectrum()` to work. This flag is read during `prepare()` when the FFT engine instance is created -- it does not retroactively change an already-created engine.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseFallback | Integer | no | Use fallback engine (true) or platform-optimized (false) | Boolean |

**Pitfalls:**
- Calling `setUseFallbackEngine(true)` after `prepare()` sets the internal flag but does not recreate the existing FFT engine. The fallback engine only activates on the next `prepare()` or automatic `reinitialise()` call. Call `setUseFallbackEngine()` before `prepare()`, or trigger reallocation (e.g., by toggling inverse FFT) after changing the flag.

**Cross References:**
- `$API.FFT.prepare$`

---

## setUseSpectrumList

**Signature:** `undefined setUseSpectrumList(Integer numRows)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new SpectrumList with the specified number of image slots.
**Minimal Example:** `{obj}.setUseSpectrumList(16);`

**Description:**
Creates an internal image list with the specified number of slots for batch spectrum collection. When active, `dumpSpectrum()` can store spectrum images by index (passing an integer instead of a File object) into this list for later batch export.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numRows | Integer | no | Number of image slots in the spectrum list | Positive integer |

**Cross References:**
- `$API.FFT.setUseFallbackEngine$`

---

## setWindowType

**Signature:** `undefined setWindowType(Integer windowType)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Triggers reinitialise() which reallocates all work buffers and recomputes the window function under a write lock.
**Minimal Example:** `{obj}.setWindowType({obj}.Hann);`

**Description:**
Sets the window function applied to each data chunk before the forward FFT transform. Use the window type constants defined on the FFT object: `Rectangle` (0), `Triangle` (1), `Hamming` (2), `Hann` (3), `BlackmanHarris` (4), `Kaiser` (5), `FlatTop` (6). The window type also affects the Spectrum2D spectrogram generation. Triggers automatic buffer reallocation if `prepare()` has been called previously.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| windowType | Integer | no | Window function type constant | 0-6 (use FFT constant properties) |

**Cross References:**
- `$API.FFT.prepare$`
- `$API.FFT.process$`
- `$API.FFT.setOverlap$`
