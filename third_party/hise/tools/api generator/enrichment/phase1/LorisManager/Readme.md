# LorisManager -- Class Analysis

## Brief
Loris partial-tracking library interface for spectral analysis, manipulation, and resynthesis of audio files.

## Purpose
LorisManager provides a scripting interface to the Loris partial-tracking library, enabling sinusoidal analysis, per-partial manipulation, and resynthesis of audio files. It wraps the Loris C API through a singleton managed by MainController, supporting both dynamic library loading and static linking. The class operates on File objects and produces Buffer arrays, Path objects, and snapshot data from the analyzed partial lists. Requires the optional `HISE_INCLUDE_LORIS` module to be enabled at compile time.

## Details

### Architecture

LorisManager is a thin scripting wrapper (`ScriptLorisManager`) around a core `LorisManager` singleton held by `MainController`. Each call to `Engine.getLorisManager()` creates a new wrapper instance, but all wrappers share the same underlying Loris state. The core LorisManager communicates with the Loris library through a pure C API, either via dynamic library loading (`HISE_USE_LORIS_DLL`) or static linking (`HISE_INCLUDE_LORIS`).

### Workflow

The typical workflow is:
1. Configure analysis options via `set()` (optional -- defaults are sensible)
2. Analyse an audio file via `analyse()` with an estimated root frequency
3. Manipulate partials via `process()` (predefined commands) or `processCustom()` (per-breakpoint callback)
4. Extract data via `synthesise()` (returns Buffer arrays), `createEnvelopes()` (parameter buffers), `createEnvelopePaths()` (Path objects), or `createSnapshot()` (harmonic values at a time point)

### File Parameter Convention

All methods accepting a `file` parameter require a `File` object (from `FileSystem`). If an invalid object is passed, methods silently return false or empty results without throwing.

### Process Commands

See `process()` for the full command table and data format per command, and `processCustom()` for the per-breakpoint callback API.

### Parameter Strings

See `createEnvelopes()` for the full list of accepted parameter strings (`"rootFrequency"`, `"frequency"`, `"phase"`, `"gain"`, `"bandwidth"`) and their value ranges. The same strings apply to `createEnvelopePaths()` and `createSnapshot()`.

### Threading

All operations call `initThreadController()` which enables progress tracking and cancellation when running on a background thread. Long-running Loris operations (analysis, processing) should be executed via `BackgroundTask` for responsive UI.

## obtainedVia
`Engine.getLorisManager()`

## minimalObjectToken
lm

## Constants
(none)

## Dynamic Constants
(none)

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `lm.process(f, "shiftPitch", 2.0)` | `lm.process(f, "shiftPitch", {"offset": 2.0})` | Process commands require JSON objects or arrays as data, not plain values. |
| `lm.analyse("path/to/file.wav", 440)` | `lm.analyse(FileSystem.fromAbsolutePath("path/to/file.wav"), 440)` | The file parameter must be a File object, not a string path. A string silently fails. |

## codeExample
```javascript
// Get the Loris manager and analyse a file
const lm = Engine.getLorisManager();
const f = FileSystem.fromAbsolutePath("path/to/sample.wav");

lm.set("timedomain", "seconds");
lm.analyse(f, 440.0);
lm.process(f, "shiftPitch", {"offset": 1.5});
const buffers = lm.synthesise(f);
```

## Alternatives
- **FFT** -- Use for standard windowed FFT spectral processing; use LorisManager for sinusoidal partial tracking with per-partial manipulation.
- **NeuralNetwork** -- Use for ML-based audio processing; use LorisManager for physics-based spectral analysis and resynthesis via Loris partials.
- **Buffer** -- Use to hold the audio data that LorisManager analyses and resynthesises; LorisManager operates on audio files and returns Buffer arrays.

## Related Preprocessors
`HISE_INCLUDE_LORIS`, `HISE_USE_LORIS_DLL`

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- LorisManager.analyse -- timeline dependency: synthesise, process, processCustom, createEnvelopes, createEnvelopePaths, createSnapshot all require a prior analyse() call (logged)
