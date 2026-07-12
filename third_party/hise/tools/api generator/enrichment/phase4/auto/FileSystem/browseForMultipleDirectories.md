Opens a native directory browser dialog that allows selecting multiple directories at once. The callback receives an Array of `File` objects, one per selected directory. Unlike `FileSystem.browse()` and `FileSystem.browseForDirectory()`, this method also accepts absolute path strings for the `startFolder` parameter.

> [!Warning:Callback always receives an Array] The callback always receives an Array, even if only one directory is selected. Iterate the result rather than treating it as a single `File`.
