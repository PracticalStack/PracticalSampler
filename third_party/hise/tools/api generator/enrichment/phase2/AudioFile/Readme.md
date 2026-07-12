# AudioFile -- Project Context

## Project Context

### Real-World Use Cases
- **Wavetable/waveform slot management**: A wavetable synthesizer uses AudioFile handles obtained via `AudioSampleProcessor.getAudioFile(0)` to load embedded waveform presets from the audio file pool. A combo box selection triggers `loadFile()` with pool reference strings returned by `Engine.loadAudioFilesIntoPool()`, enabling instant waveform switching without filesystem access.
- **Drum machine sample slot monitoring**: A multi-channel drum machine with dozens of audio file slots uses `Broadcaster.attachToComplexData("AudioFile.Content", ...)` to watch all sample slots for content changes. This drives UI updates (waveform displays, sample name labels), missing file detection, and lazy loading workflows - all without registering individual `setContentCallback()` on each AudioFile.
- **Impulse response management**: An FX plugin uses AudioFile monitoring via Broadcaster to keep a waveform display synchronized with the currently loaded IR file, triggering UI refresh whenever the convolution processor's audio slot content changes.

### Complexity Tiers
1. **Basic file loading** (most common): `AudioSampleProcessor.getAudioFile(0)` + `loadFile()`. Sufficient for loading pool-referenced audio files into processor slots (wavetables, IRs, one-shots).
2. **Content change monitoring**: Add `Broadcaster.attachToComplexData("AudioFile.Content", moduleId, slotIndex, ...)` to react to file loads and content changes across one or more processors. This is the preferred pattern over `setContentCallback()` because a single broadcaster can watch multiple processor slots simultaneously.
3. **Programmatic buffer injection**: Use `loadBuffer()` to inject script-generated or processed audio data (synthesized waveforms, recorded audio) into processor slots. Combine with `getContent()` and `update()` for read-modify-write workflows on the underlying sample data.

### Practical Defaults
- Use `Broadcaster.attachToComplexData("AudioFile.Content", moduleId, 0, "description")` rather than `setContentCallback()` when monitoring audio file changes. The broadcaster approach scales to multiple slots, supports delayed listeners, and integrates with the broader event bus architecture.
- Use `Engine.loadAudioFilesIntoPool()` at init time to get an array of pool reference strings, then pass these to `loadFile()`. This is the standard pattern for offering a selection of embedded audio files (waveform presets, IR libraries).
- Obtain AudioFile handles via `Synth.getAudioSampleProcessor(id).getAudioFile(slotIndex)` rather than `Engine.createAndRegisterAudioFile(index)`. The former binds to a specific processor's audio slot, ensuring that loading a file actually affects the processor's playback. The standalone creation method is for script-owned data slots not tied to a specific processor.

### Integration Patterns
- `Synth.getAudioSampleProcessor(id)` -> `AudioSampleProcessor.getAudioFile(0)` -> `AudioFile.loadFile()` - Standard chain for loading audio into a processor slot (AudioLooper, ConvolutionEffect, WavetableSynth).
- `Engine.loadAudioFilesIntoPool()` -> `AudioFile.loadFile(poolRef)` - Load embedded audio files by pool reference string. The pool list provides valid reference strings that `loadFile()` accepts.
- `Broadcaster.attachToComplexData("AudioFile.Content", moduleIds, 0, ...)` -> `addComponentRefreshListener()` / `addComponentPropertyListener()` - Watch audio content changes and drive UI updates (repaint waveform displays, update file name labels).
- `FileSystem.browse(FileSystem.AudioFiles, ...)` -> `AudioFile.loadFile(file.toString(0))` - User file browsing workflow. The browse callback returns a File object; convert to string for `loadFile()`.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Registering `setContentCallback()` on each of 12+ AudioFile handles individually | Using one `Broadcaster.attachToComplexData("AudioFile.Content", arrayOfModuleIds, 0, ...)` | A single broadcaster can monitor all processor slots at once, with the processor ID passed as a callback argument for routing. Individual callbacks create redundant boilerplate and miss the broadcaster's built-in features (delayed listeners, queuing, metadata). |
| Calling `loadFile()` with a filesystem path like `"C:/audio/file.wav"` | Using a pool reference like `"{PROJECT_FOLDER}file.wav"` or converting via `file.toString(0)` | `loadFile()` expects a HISE pool reference string or the output of `File.toString(0)`, not a raw filesystem path. |
