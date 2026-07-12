# FFT -- Class Analysis

## Brief
Windowed FFT processor for spectral analysis, magnitude/phase callbacks, inverse resynthesis, and 2D spectrogram generation.

## Purpose
The FFT class provides a scriptable Fast Fourier Transform processor that operates on Buffer objects. It supports windowed overlap-add processing with configurable magnitude and phase callbacks, optional inverse FFT for spectral resynthesis, and 2D spectrogram image generation via the Spectrum2D subsystem. Multi-channel processing is supported with up to 16 channels. The class wraps JUCE's `juce::dsp::FFT` engine and provides a high-level chunked processing pipeline with overlap control.

## Details

### Processing Pipeline

The `process()` method runs two independent code paths that can both be active simultaneously:

1. **Spectrum2D path** (when `setEnableSpectrum2D(true)`): Generates a spectrogram image from the input buffer. This executes before the callback path.
2. **Callback path** (when magnitude/phase functions are set): Processes data in overlapping chunks, calling user-defined callbacks with magnitude and/or phase buffers, then optionally reconstructing the signal via inverse FFT.

### Overlap-Add Processing

When overlap is configured via `setOverlap()`, the processor steps through the input in chunks of `fftSize * (1.0 - overlap)` samples. Each chunk is windowed, transformed, passed to callbacks, and (if inverse is enabled) reconstructed and overlap-added into the output buffer. The first chunk skips the first quarter of the window to avoid startup edge artifacts.

### Callback Signature

Both magnitude and phase callbacks receive two arguments -- see `setMagnitudeFunction()` and `setPhaseFunction()` for full callback details including decibel conversion and multi-channel behavior.

### Spectrum2D Parameters (JSON)

See `setSpectrum2DParameters()` for the full property schema and `getSpectrum2DParameters()` for retrieval.

### Buffer Allocation Strategy

`prepare()` allocates per-channel work buffers conditionally:
- `chunkInput` (2x FFT size): always allocated
- `chunkOutput` (2x FFT size): only when inverse FFT is enabled
- `magBuffer` (FFT size): only when a magnitude function is set or inverse is enabled
- `phaseBuffer` (FFT size): only when a phase function is set or inverse is enabled

Changing configuration after `prepare()` (e.g., calling `setMagnitudeFunction()` or `setEnableInverseFFT()`) triggers automatic re-allocation via `reinitialise()`.

### Fallback Engine

See `setUseFallbackEngine()` for details on forcing the JUCE fallback FFT implementation (required for `dumpSpectrum()`).

### Spectrum List (Batch Export)

See `setUseSpectrumList()` for batch spectrum image collection via `dumpSpectrum()`.

## obtainedVia
`Engine.createFFT()`

## minimalObjectToken
fft

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| Rectangle | 0 | int | Rectangular (no) window function | WindowType |
| Triangle | 1 | int | Triangular window function | WindowType |
| Hamming | 2 | int | Hamming window function | WindowType |
| Hann | 3 | int | Hann window function | WindowType |
| BlackmanHarris | 4 | int | Blackman-Harris window function | WindowType |
| Kaiser | 5 | int | Kaiser window function | WindowType |
| FlatTop | 6 | int | Flat-top window function | WindowType |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `fft.process(buf)` without calling `fft.prepare()` first | Call `fft.prepare(1024, 1)` before `fft.process(buf)` | process() checks that prepare() was called and throws "You must call prepare before process" if buffers are not allocated. |
| `fft.prepare(1024, 1)` then `fft.setMagnitudeFunction(fn, false)` then `fft.process(buf)` | Set magnitude/phase functions before calling `prepare()`, or accept that `setMagnitudeFunction` triggers automatic re-prepare | Setting callbacks after prepare triggers `reinitialise()` which re-allocates all buffers. While this works, it is an unnecessary double allocation. However, the real bug is calling prepare with a non-power-of-two size, which throws an error. |
| `fft.dumpSpectrum(file, true, 512, 256)` without fallback engine | Call `fft.setUseFallbackEngine(true)` before `fft.prepare()` | dumpSpectrum requires the fallback FFT engine. Without it, you get "You must use the fallback engine if you want to dump FFT images". |

## codeExample
```javascript
// Create an FFT processor and analyse a buffer
const var fft = Engine.createFFT();

fft.setWindowType(fft.Hann);
fft.setOverlap(0.5);
fft.setMagnitudeFunction(function(magnitudes, offset)
{
    // magnitudes is a Buffer with frequency bin amplitudes
    // offset is the current position in the source buffer
}, false);

fft.prepare(1024, 1);
```

## Alternatives
- `LorisManager` -- partial-tracking analysis and resynthesis (more advanced spectral modeling)
- `NeuralNetwork` -- machine-learning inference on audio data
- `Buffer` -- holds the audio data that FFT processes

## Related Preprocessors
`USE_BACKEND` -- realtime safety checks for magnitude/phase callbacks (IDE only).

## Diagrams

### fft-processing-pipeline
- **Brief:** FFT Processing Pipeline
- **Type:** topology
- **Description:** Input Buffer -> Window Function -> Forward FFT -> [Magnitude Callback] -> [Phase Callback] -> [Inverse FFT (if enabled)] -> Output Buffer. Parallel path: Input Buffer -> Spectrum2D -> Spectrum Image. The callback path processes in overlapping chunks with configurable overlap. The inverse path uses overlap-add reconstruction.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: The two callbacks (magnitudeFunction, phaseFunction) already have ADD_CALLBACK_DIAGNOSTIC registrations in the constructor. The only other precondition (prepare-before-process) produces an immediate runtime error, not a silent failure.
