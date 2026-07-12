Returns an Array of `File` objects for all files and directories within a folder that match the given wildcard pattern. Use this to build a file browser, scan a sample library, or enumerate presets in a directory. The first parameter must be a `File` object - pass the result of `FileSystem.getFolder()`, not a `SpecialLocations` constant directly. Set `recursive` to `true` to include subdirectories.

> [!Warning:Pass File object, not SpecialLocations constant] Passing a `SpecialLocations` constant (e.g. `FileSystem.Desktop`) instead of a `File` object silently returns an empty array. Always call `FileSystem.getFolder()` first.

> [!Warning:Cache results for large directories] On large content libraries, recursive scanning can take noticeable time. Cache the results and only rescan when the user explicitly requests it.
