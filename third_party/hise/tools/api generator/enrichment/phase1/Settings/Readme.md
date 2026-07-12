# Settings -- Class Analysis

## Brief
Audio device, MIDI input, zoom, disk mode, and OpenGL configuration namespace for standalone and plugin builds.

## Purpose
Settings is a global namespace that provides get/set access to audio driver configuration, MIDI input devices, UI zoom level, disk streaming mode, OpenGL rendering, and diagnostic tools. It delegates to three MainController subsystems: GlobalSettingManager (zoom, disk mode, voice multiplier, OpenGL state), AudioProcessorDriver (audio device selection, buffer size, sample rate, output channels, MIDI inputs), and MainController directly (MIDI channel filtering, debug logging, MIDI learn). Many audio device methods are primarily useful in standalone builds where the application manages its own audio hardware; in plugin builds, the host controls the audio device.

## Details

### Infrastructure Delegation

Settings does not own any state. It casts the MainController into three interface pointers at construction time and delegates all calls:

| Subsystem | Pointer | Methods |
|-----------|---------|---------|
| GlobalSettingManager | `gm` | getZoomLevel, setZoomLevel, getDiskMode, setDiskMode, getCurrentVoiceMultiplier, setVoiceMultiplier, isOpenGLEnabled, setEnableOpenGL |
| AudioProcessorDriver | `driver` | All audio device methods (get/set device, buffer size, sample rate, output channels), MIDI input toggle/query |
| MainController | `mc` | clearMidiLearn, setEnableDebugMode, toggleMidiChannel, isMidiChannelEnabled |

### Audio Device Null Pattern

When no audio device is selected, getter methods return predictable defaults:
- Array getters (`getAvailableBufferSizes`, `getAvailableOutputChannels`, etc.) return empty arrays
- String getters (`getCurrentAudioDevice`, `getCurrentAudioDeviceType`) return empty strings
- `getCurrentSampleRate` returns -1
- `getCurrentOutputChannel` returns 0

### Zoom Clamping

See `setZoomLevel` for clamping range details. Values are clamped to [0.25, 2.0].

### MIDI Channel Indexing

See `toggleMidiChannel` and `isMidiChannelEnabled` for the 0-based special index convention (0 = all channels, 1-16 = individual).

### Disk Mode Values

| Value | Mode | Meaning |
|-------|------|---------|
| 0 | SSD | Fast storage -- larger preload buffer |
| 1 | HDD | Slow storage -- stream-optimized |

### OpenGL Deferred Application

See `setEnableOpenGL` for deferred-application behavior. `isOpenGLEnabled` may be out of sync with actual rendering state until the next interface rebuild.

### setSampleFolder Backend/Frontend Split

See `setSampleFolder` for the backend/frontend behavior difference. The parameter must be a `File` object (not a string path).

### isIppEnabled Platform Logic

See `isIppEnabled` for the Windows/macOS detection logic. This method answers "is fast FFT available?" -- macOS always has vDSP, while Windows needs IPP.

### Perfetto Tracing

See `startPerfettoTracing` and `stopPerfettoTracing`. Requires `PERFETTO=1` at compile time.

### crashAndBurn Safety

See `crashAndBurn`. Requires `CompileWithDebugSymbols` in backend builds for meaningful crash reports.

## obtainedVia
Global namespace -- always available as `Settings` in all script contexts.

## minimalObjectToken


## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Settings.setZoomLevel(3.0);` | `Settings.setZoomLevel(2.0);` | Zoom is clamped to [0.25, 2.0]. Values outside this range are silently clamped, not rejected. |
| `Settings.setSampleFolder("/path/to/samples");` | `Settings.setSampleFolder(FileSystem.getFolder(FileSystem.Samples));` | setSampleFolder requires a File object, not a string path. |
| `Settings.stopPerfettoTracing(traceFile);` (without PERFETTO=1) | Compile with `PERFETTO=1` first | Perfetto methods report a script error if the project was not compiled with Perfetto support. |
| `Settings.toggleMidiChannel(1, true);` expecting "all channels" | `Settings.toggleMidiChannel(0, true);` | Index 0 controls all channels; indices 1-16 are individual MIDI channels. |

## codeExample
```javascript
// Settings is a global namespace -- no instantiation needed.
// Query current audio configuration
var sampleRate = Settings.getCurrentSampleRate();
var bufferSize = Settings.getCurrentBufferSize();
var zoomLevel = Settings.getZoomLevel();
```

## Alternatives
Engine -- handles project-level concerns (presets, resources, conversions) while Settings manages audio device and app configuration.

## Related Preprocessors
`PERFETTO`, `USE_BACKEND`, `USE_IPP`, `HISE_USE_OPENGL_FOR_PLUGIN`

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Settings methods are simple get/set wrappers with no precondition chains, timeline dependencies, or silent-failure modes that would benefit from parse-time diagnostics.
