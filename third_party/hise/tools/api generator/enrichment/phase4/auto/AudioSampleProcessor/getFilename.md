Returns the pool reference string of the currently loaded file (e.g. `{PROJECT_FOLDER}loop.wav`). Returns an empty string if no file is loaded - check `.length` as a quick loaded/unloaded test.

> [!Warning:Returns pool reference, not filesystem path] The returned string is a pool reference, not a filesystem path. Use `FileSystem.fromReferenceString()` to convert it to a File object for directory browsing or path operations.
