# AudioSampleProcessor -- Project Context

## Project Context

### Real-World Use Cases
- **Drum machine sample player**: A multi-pad drum machine uses arrays of AudioSampleProcessor handles (one per pad layer) to load user-imported one-shot samples at runtime. The handles are created in a loop during init and stored in an array for indexed access from sequencer callbacks and preset serialization.
- **Convolution reverb IR loader**: An FX plugin wraps its Convolution Reverb module with an AudioSampleProcessor handle to load impulse response files from a browsable list. The `setFile()` / `getFilename()` pair drives the IR selection UI and preset recall.
- **Wavetable source bridge**: A wavetable synthesizer uses `getAudioSampleProcessor(wavetableId).getAudioFile(0)` to bridge from the module handle to the AudioFile complex data API, enabling drag-and-drop wavetable import and broadcaster-driven change detection.

### Complexity Tiers
1. **Basic IR/sample loading** (most common): `setFile()` and `getFilename()` to load audio files and query the current file. Covers convolution reverb IR selection, simple audio loop playback.
2. **Batch operations with arrays**: Creating arrays of AudioSampleProcessor handles in a loop for multi-pad or multi-layer instruments. `getFilename()` for preset serialization across all slots.
3. **AudioFile bridge and change detection**: Using `getAudioFile(0)` to access the complex data API for broadcaster attachment (`attachToComplexData`), file drop targets, and programmatic buffer manipulation.

### Practical Defaults
- Use `{PROJECT_FOLDER}` references for audio files bundled with the plugin. Use absolute paths (from `File.toString(0)`) for user-imported files.
- Use `getFilename().length` as a quick check for whether a file is currently loaded -- an empty string means no file.
- When building a file browser that should open in the current file's directory, convert the pool reference to a File object: `FileSystem.fromReferenceString(asp.getFilename(), FileSystem.AudioFiles).getParentDirectory()`.

### Integration Patterns
- `AudioSampleProcessor.getAudioFile(0)` -> `AudioFile` API -- bridges the module handle to the complex data system for broadcaster attachment, sample data access, and file callbacks.
- `AudioSampleProcessor.getFilename()` -> `FileSystem.fromReferenceString()` -- converts a pool reference string back to a File object for directory browsing, file comparison, and UI display.
- `Broadcaster.attachToComplexData("AudioFile.Content", moduleIds, 0)` -- monitors file changes across multiple AudioSampleProcessor modules for missing-file detection or UI updates.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `asp.getFilename()` used directly as a file path | `FileSystem.fromReferenceString(asp.getFilename(), FileSystem.AudioFiles)` | `getFilename()` returns a pool reference string (e.g., `{PROJECT_FOLDER}loop.wav`), not a filesystem path. Convert it to a File object before any file system operations. |
| Using `{PROJECT_FOLDER}` for user-imported files | Use `file.toString(0)` (absolute path) from the browse callback | `{PROJECT_FOLDER}` resolves to the project's AudioFiles folder. User-imported files from arbitrary disk locations need absolute paths. |
