Opens a native file browser dialog that allows selecting multiple files at once. The callback receives an Array of `File` objects, one per selected file. This method always opens an open-file dialog (never save). Like `FileSystem.browseForMultipleDirectories()`, it also accepts absolute path strings for the `startFolder` parameter.

> [!Warning:Callback always receives an Array] The callback always receives an Array, even if only one file is selected. Iterate the result rather than treating it as a single `File`.
