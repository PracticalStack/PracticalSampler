# FileSystem -- Project Context

## Project Context

### Real-World Use Cases
- **Settings persistence**: A plugin that needs to persist user preferences (theme, zoom level, MIDI mappings) across sessions uses `getFolder(FileSystem.AppData)` to locate a stable settings file, then chains `.getChildFile("settings.json")` for read/write with `File.loadAsObject()` / `File.writeObject()`. This is the most common FileSystem pattern - nearly every plugin that stores anything beyond presets uses it.
- **Sample content browser**: A plugin with user-importable audio content uses `getFolder(FileSystem.Samples)` as the root, `findFiles()` to recursively scan subdirectories, and builds an in-memory database of available sounds with metadata extracted from the directory structure (category/type/filename). Results are cached to AppData as JSON to avoid re-scanning on every launch.
- **Preset library management**: A plugin with a custom preset browser uses `getFolder(FileSystem.UserPresets)` to enumerate `.preset` files, `findFiles()` to scan subdirectories for category organization, and `fromAbsolutePath()` to reconstruct `File` handles from stored path strings. Favorites are stored as a JSON array of relative paths in the UserPresets directory.
- **Custom sample import**: A plugin that lets users load their own audio files uses `browse()` to open a native file dialog, then loads the selected file into an `AudioSampleProcessor`. The start folder is set intelligently - defaulting to the last-used directory when available, or falling back to `FileSystem.Samples`.

### Complexity Tiers
1. **Basic file access** (most common): `getFolder()` + `getChildFile()` for accessing well-known paths. Nearly every plugin uses this for settings, favorites, or cache files in AppData.
2. **Content scanning**: `findFiles()` with wildcard filtering for building file lists. Used by any plugin with a sample browser, IR selector, or preset explorer. Requires understanding the `getFolder()` -> `File` object prerequisite.
3. **File dialogs**: `browse()`, `browseForDirectory()` for user-initiated file operations (import audio, save/load custom settings, relocate sample folders). Requires understanding the async callback pattern and the re-entry guard.
4. **Reference string resolution**: `fromReferenceString()` for resolving `{PROJECT_FOLDER}` pool references to disk files. Used when converting between HISE's internal reference format and absolute paths for display or file operations.
5. **Encryption utilities**: `getSystemId()` for machine identification (crash reporting, analytics), `encryptWithRSA()` / `decryptWithRSA()` for secure data exchange (signed configuration files, tamper-proof settings).

### Practical Defaults
- Use `FileSystem.AppData` for any persistent data that is not a preset (settings, caches, favorites, MIDI mappings). This resolves to the platform-appropriate application data folder and is stable across plugin versions.
- Use `FileSystem.Samples` as the start folder for browse dialogs when the user is selecting audio content. It opens at the expected content location and avoids navigation friction.
- When scanning directories with `findFiles()`, cache the results to a JSON file in AppData for large content libraries. Re-scan only when the user explicitly requests it or when a content folder change is detected.
- When using `browse()`, pass a `File` object for the start folder rather than a `SpecialLocations` constant when you want to remember the last-used directory. Store the last directory path in your settings file and reconstruct it with `fromAbsolutePath()`.
- Always check `isDefined()` on the result of `getFolder()` before chaining methods. HISE-managed locations (`Samples`, `AudioFiles`) can return `undefined` if the folder does not exist on disk.

### Integration Patterns
- `FileSystem.getFolder()` -> `File.getChildFile()` -> `File.loadAsObject()` / `File.writeObject()` - The standard settings persistence chain. Get the AppData folder, navigate to a specific file, read/write JSON.
- `FileSystem.getFolder()` -> `FileSystem.findFiles()` -> `File.toString()` / `File.getParentDirectory()` - The content scanning chain. Get a root folder, enumerate files recursively, extract metadata from paths.
- `FileSystem.browse()` -> callback `File` -> `AudioSampleProcessor.setFile()` - The sample import chain. Browse for an audio file, then load it into a processor.
- `FileSystem.fromReferenceString()` -> `File.toString()` / `File.getParentDirectory()` - Convert a HISE pool reference (from `AudioSampleProcessor.getFilename()`) back to a `File` for display or navigation.
- `FileSystem.getSystemId()` -> `Server.callWithPOST()` - Include a unique machine identifier in server requests for crash reporting or analytics.
- `FileSystem.browseForDirectory()` -> `Settings.setSampleFolder()` - The sample folder relocation chain. Let the user choose a new content directory, validate it, then update the engine setting.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Storing absolute paths in presets or saved state | Store relative paths using `File.toString(1)` and reconstruct with `fromAbsolutePath()` or `getFolder().getChildFile()` | Absolute paths break when the plugin is used on a different machine or when the sample folder is relocated. |
| Calling `findFiles()` on every UI refresh | Cache the file list in a variable or JSON file, rescan only on explicit user action | Directory scanning is expensive, especially on network drives or large content libraries. |
| Using `browse()` with `"*"` as the wildcard for audio import | Use `"*.wav,*.aif"` or a specific audio wildcard | A wildcard of `"*"` shows all files including system files, making the dialog harder to navigate. |
| Assuming `getFolder(FileSystem.Samples)` always returns a valid folder | Check `isDefined()` before using the result | The Samples folder can be undefined if FullInstrumentExpansion is active with no expansion loaded, or if the sample folder link is missing. |
