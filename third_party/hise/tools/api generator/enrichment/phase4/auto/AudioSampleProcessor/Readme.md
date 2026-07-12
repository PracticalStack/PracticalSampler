<!-- Diagram triage:
  - (none): No diagram specifications in Phase 1 data
-->

# AudioSampleProcessor

AudioSampleProcessor is a script handle to any module that owns an audio file slot. It wraps these module types:

- Audio Loop Player - single-file playback with loop and pitch tracking
- Convolution Reverb - impulse response convolution
- Wavetable Synthesiser - wavetable data source
- Noise Grain Player - granular noise residual playback

Use it to load audio files from the pool, control the active sample range and loop points, and read or write the module's parameters. For deeper access to the audio data itself (sample buffers, content change callbacks, drag-and-drop targets), bridge to the `AudioFile` complex data API via `getAudioFile(0)`.

```js
const var asp = Synth.getAudioSampleProcessor("AudioLooper1");
```

Audio files are loaded using pool reference strings rather than filesystem paths. Use `{PROJECT_FOLDER}` for files in the project's AudioFiles folder and `{EXP::expansionName}` for expansion content. The AudioFilePool floating tile provides a browser interface for previewing and managing pooled audio files.

Each handle exposes the wrapped module's parameters as dynamic integer constants (e.g. `asp.Gain`, `asp.SyncMode`). The available constants depend on the module type - use `getNumAttributes()` and `getAttributeId()` to discover them at runtime.

## Complex Data Chain

Audio file workflows use a three-part complex-data chain:

![Audio File Data Chain](topology_complex-audio-data-chain.svg)

- `AudioSampleProcessor` selects the module that owns one or more audio file slots.
- `AudioFile` is the complex-data handle for one slot within that module.
- `ScriptAudioWaveform` displays or edits one selected slot in the UI.

Use the binding properties separately:

- `processorId` selects the owning processor.
- `sampleIndex` selects which audio slot inside that processor should be displayed.

This is not the normal parameter binding path. `parameterId` targets processor parameters, while audio-slot binding uses `sampleIndex` instead.

> [!Tip:Standard module control methods available] AudioSampleProcessor also provides the standard module control methods (`getAttribute`, `setAttribute`, `setBypassed`) shared with other module handle classes such as Effect, Modulator, and ChildSynth.

## Common Mistakes

- **Use pool references not absolute paths**
  **Wrong:** `asp.setFile("C:/audio/file.wav");`
  **Right:** `asp.setFile("{PROJECT_FOLDER}file.wav");`
  *`setFile` expects a pool reference string with wildcards, not an absolute filesystem path.*

- **Load pool before setFile in IDE**
  **Wrong:** Calling `setFile` in the HISE IDE without loading the pool first
  **Right:** Call `Engine.loadAudioFilesIntoPool()` before `setFile`
  *In the HISE IDE, audio files must be loaded into the pool before they can be referenced. Exported plugins have embedded files and do not need this step.*

- **Convert pool reference to file path**
  **Wrong:** Using `asp.getFilename()` directly as a file path
  **Right:** `FileSystem.fromReferenceString(asp.getFilename(), FileSystem.AudioFiles)`
  *`getFilename()` returns a pool reference string (e.g. `{PROJECT_FOLDER}loop.wav`), not a filesystem path. Convert it to a File object before any file system operations.*

- **Use absolute paths for imported files**
  **Wrong:** Using `{PROJECT_FOLDER}` for user-imported files
  **Right:** Use `file.toString(0)` (absolute path) from the browse callback
  *`{PROJECT_FOLDER}` resolves to the project's AudioFiles folder. User-imported files from arbitrary locations need absolute paths.*
