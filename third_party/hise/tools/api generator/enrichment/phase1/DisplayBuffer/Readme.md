# DisplayBuffer -- Class Analysis

## Brief
Scriptable reference to a processor's ring buffer for real-time audio visualization and state serialization.

## Purpose
DisplayBuffer provides script-level access to a SimpleRingBuffer owned by a DSP processor, enabling real-time waveform, spectrum, and envelope visualization. It reads audio data written by the audio thread, converts it to paths or resampled buffers for UI rendering, and supports property-based configuration of the underlying display type (FFT, oscilloscope, goniometer, envelope). The buffer behavior is polymorphic -- properties and path generation adapt automatically to the source processor type via the PropertyObject system.

## Details

### Architecture

DisplayBuffer wraps a `SimpleRingBuffer`, which is one of four complex data types in HISE's `ExternalData` system (alongside Table, SliderPack, and AudioFile). The scripting API class name is `DisplayBuffer` but the C++ class is `ScriptRingBuffer`. It inherits from `ScriptComplexDataReferenceBase`, sharing the same base infrastructure as AudioFile, Table, and SliderPackData handles.

### PropertyObject System

The ring buffer's behavior is determined by a polymorphic `PropertyObject` attached to the underlying `SimpleRingBuffer`. Each DSP node type that writes to a display buffer registers its own PropertyObject subclass, which controls:

- Buffer size validation and constraints
- Read buffer transformation (e.g., FFT windowing, envelope state copying)
- Path generation for visualization
- Available configuration properties
- State serialization (base64 export/import)

The PropertyObject is set automatically when a DSP node connects as a writer. The `setRingBufferProperties()` method passes JSON key-value pairs to the active PropertyObject.

### PropertyObject Types and Their Properties

| Source Type | Available Properties |
|-------------|---------------------|
| Generic (default) | BufferLength, NumChannels, Active |
| FFT Analyser | BufferLength, WindowType, Overlap, DecibelRange, UsePeakDecay, UseDecibelScale, YGamma, Decay, UseLogarithmicFreqAxis |
| Oscilloscope | BufferLength (128-65536), NumChannels (1-2) |
| Goniometer | BufferLength (512-32768), NumChannels (fixed 2) |
| Modulator Plotter | Fixed size, internal transform |
| Envelope (AHDSR/AR/Flex) | Fixed size, envelope state display |
| Oscillator Display | Fixed size, waveform shape |

### Read Buffer vs Write Buffer

The SimpleRingBuffer maintains separate internal and read buffers. The audio thread writes to the internal buffer via atomic indices. The read buffer is a snapshot prepared for UI consumption. See `getReadBuffer()` for direct reference access, `copyReadBuffer()` for thread-safe copies, and `getResizedBuffer()` for resampled copies. `createPath()` acquires a read lock for safe path generation.

### createPath Source Range Encoding

The `sourceRange` parameter for `createPath()` uses a non-standard packing: `[minValue, maxValue, startSample, endSample]` -- the first two elements define the value normalization range and the last two define the sample range. See `createPath()` for full details.

## obtainedVia
`Engine.createAndRegisterRingBuffer(index)` or `Synth.getDisplayBufferSource(moduleId).getDisplayBuffer(index)`

## minimalObjectToken
db

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var buf = db.getReadBuffer(); buf[0] = 1.0;` | `var buf = Buffer.create(size); db.copyReadBuffer(buf);` | getReadBuffer() returns a direct reference to shared memory. Modifying it corrupts the ring buffer. Use copyReadBuffer() for a safe copy. |
| `db.copyReadBuffer(Buffer.create(512))` when buffer size is 8192 | `db.copyReadBuffer(Buffer.create(8192))` | copyReadBuffer requires the target buffer size to match exactly. Check the read buffer size first or use getResizedBuffer() for resampling. |

## codeExample
```javascript
// Get a display buffer from an analyser module
const var src = Synth.getDisplayBufferSource("Analyser1");
const var db = src.getDisplayBuffer(0);

// Configure FFT properties
db.setRingBufferProperties({
    "BufferLength": 8192,
    "WindowType": "BlackmanHarris",
    "UseDecibelScale": true
});
```

## Alternatives
- **Table** -- Use Table for editable modulation curve data; use DisplayBuffer for read-only ring buffer visualization from a DSP processor.
- **SliderPackData** -- Use SliderPackData for editable discrete slider values; use DisplayBuffer for streaming audio visualization data.
- **AudioFile** -- Use AudioFile for loading/managing audio files; use DisplayBuffer for visualizing a processor's real-time audio output.
- **Buffer** -- Use Buffer for standalone sample manipulation; use DisplayBuffer for accessing a processor's internal ring buffer for UI visualization.

## Related Preprocessors
None.

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: DisplayBuffer methods operate on runtime data with clear error reporting (size mismatch in copyReadBuffer, rectangle parsing errors in createPath). No silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics.
