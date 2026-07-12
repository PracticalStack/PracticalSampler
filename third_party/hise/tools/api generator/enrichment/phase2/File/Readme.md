# File -- Project Context

## Project Context

### Real-World Use Cases
- **Custom preset and settings persistence**: Plugins that implement custom preset browsers or save user-specific settings (theme, MIDI mappings, routing configurations) use File objects to read and write JSON data alongside the UserPresets or AppData directories. The JSON round-trip pattern (`loadAsObject` / `writeObject`) is the most common File usage pattern - it serves as the primary mechanism for persisting structured data outside the built-in user preset system.
- **Sample installation and ZIP extraction**: Plugins that ship large sample libraries as separate downloads use `extractZipFile` with progress tracking to install samples into a user-selected directory, then write a HISE link file (`LinkWindows` / `LinkOSX`) using `writeString` to redirect the Samples folder.
- **Audio file preview and export**: Plugins that allow previewing or exporting audio use `loadAsAudioFile` and `loadAudioMetadata` for preview playback, and `writeAudioFile` with `getNonExistentSibling` for export to avoid overwriting existing files.
- **XML settings storage**: Plugins that need to persist a small amount of configuration outside the preset system (e.g., graphics quality level) use the `writeAsXmlFile` / `loadFromXmlFile` round-trip for settings that should survive preset changes.
- **Pool reference generation**: Plugins that manage user-imported audio files convert absolute File paths to pool reference strings via `toReferenceString("AudioFiles")` for use with the HISE audio file pool.

### Complexity Tiers
1. **Basic persistence** (most common): `loadAsObject`, `writeObject`, `getChildFile`, `isFile`, `deleteFileOrDirectory`, `toString`. Every plugin that persists custom data uses this tier.
2. **File management**: `createDirectory`, `getParentDirectory`, `getNonExistentSibling`, `getRedirectedFolder`, `toReferenceString`, `loadAudioMetadata`. Plugins with custom file browsers or sample management.
3. **Installation workflows**: `extractZipFile`, `getSize`, `getBytesFreeOnVolume`, `writeString` (for link files), `hasWriteAccess`. Plugins that include a sample installer.
4. **Audio/MIDI I/O**: `loadAsAudioFile`, `writeAudioFile`, `loadAsMidiFile`, `writeMidiFile`. Plugins that export or preview audio/MIDI content.

### Practical Defaults
- Use `FileSystem.getFolder(FileSystem.AppData).getChildFile("settings.json")` as the base path for persistent settings that should survive preset changes.
- Always check `isDefined()` on the result of `loadAsObject` or `loadFromXmlFile` before accessing properties - the file may not exist yet on first run.
- Use `getNonExistentSibling()` before `writeAudioFile` when exporting to avoid silently overwriting existing files.
- For sample installer workflows, check `getBytesFreeOnVolume()` on the target drive before starting extraction.
- Use the `toString(1)` format (NoExtension) when you need a display name without the file extension, and `toString(0)` (FullPath) when constructing lookup keys or storing paths.

### Integration Patterns
- `FileSystem.getFolder()` -> `File.getChildFile()` -> `File.loadAsObject()` - The standard chain for loading persistent configuration from a known location.
- `FileSystem.browse()` callback -> `File.loadAsObject()` / `File.writeObject()` - User-initiated file import/export through the OS file dialog.
- `File.loadAsAudioFile()` + `File.loadAudioMetadata()` -> `Engine.playBuffer()` - Audio preview workflow: load both sample data and metadata (for sample rate), then play through the engine.
- `File.extractZipFile()` -> `File.writeString()` (link file) - Sample installation: extract ZIP, then write a link file to redirect the Samples folder.
- `File.toReferenceString("AudioFiles")` -> `AudioSampleProcessor.setFile()` - Convert a user-imported file to a pool reference for use with audio player modules.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var data = f.loadAsObject();` then immediately access `data.SomeKey` | `var data = f.loadAsObject(); if (!isDefined(data)) data = {};` | On first run the file does not exist, and `loadAsObject` returns undefined. Always provide a fallback. |
| Calling `deleteFileOrDirectory()` then using the same File object for the new file | Use the target File object or obtain a fresh handle | The File handle still points to the deleted path. The underlying path is immutable after construction. |
| Using `toString(0)` as a portable key across machines | Use `getRelativePathFrom(baseDir)` or `toReferenceString(folderType)` | Full paths are machine-specific. Relative paths or pool references are portable across installations. |
