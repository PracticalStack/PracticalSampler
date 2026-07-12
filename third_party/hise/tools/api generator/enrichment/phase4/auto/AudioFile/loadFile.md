Loads an audio file into the slot from a HISE pool reference string (e.g. `{PROJECT_FOLDER}loop.wav`). Pass an empty string to clear the slot. When loading a file selected through `FileSystem.browse()`, convert the result with `file.toString(0)` since this method expects a string, not a File object.

> [!Warning:Use pool references, not file paths] Despite the parameter name `filePath`, this method expects a pool reference string in most workflows. Use `Engine.loadAudioFilesIntoPool()` to get valid reference strings for embedded audio files.
