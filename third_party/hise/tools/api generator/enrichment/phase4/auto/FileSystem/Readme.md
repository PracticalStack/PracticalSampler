<!-- Diagram triage:
  - No diagram specs in Phase 1 data. Nothing to render.
-->

# FileSystem

FileSystem is a global namespace for accessing the filesystem from HiseScript. It provides `File` objects for well-known system and project directories, opens native file browser dialogs, scans directories for content, and offers RSA encryption utilities for secure data exchange.

The namespace resolves **special folder locations** via constants that you pass to `FileSystem.getFolder()`:

| Constant | Description |
|----------|-------------|
| `AudioFiles` | Non-streaming audio files (impulse responses, loops). In the IDE this points to the project subfolder; in exported plugins it requires special build configuration. |
| `Expansions` | Expansion pack root folder. |
| `Samples` | Sample files used by the streaming engine. |
| `UserPresets` | User preset storage directory. |
| `AppData` | Application data directory (Company/Product) - the main directory for configuration files and persistent data. |
| `UserHome` | User home folder. |
| `Documents` | User documents folder. |
| `Desktop` | User desktop folder. |
| `Downloads` | User downloads folder. |
| `Applications` | Global applications directory. |
| `Temp` | System temporary directory. |
| `Music` | User music directory. |

The typical workflow is to call `FileSystem.getFolder()` to obtain a `File` object, then navigate from there with `File.getChildFile()`. For file browser dialogs, all four browse methods run asynchronously and deliver results via a callback function - they never return a value directly.

> [!Tip:Prefer AppData over personal folders] Using any of the user's personal folders (`UserHome`, `Documents`, `Desktop`, `Downloads`) without a good reason is bad practice and should be avoided. Prefer `AppData` for persistent plugin data.

> [!Tip:HISE locations resolve differently per build] HISE-managed locations (`Samples`, `AudioFiles`, `Expansions`, `UserPresets`, `AppData`) resolve to different paths depending on whether you are running in the HISE IDE or an exported plugin. OS-mapped locations (`UserHome`, `Desktop`, `Documents`, etc.) resolve identically in all builds.

> [!Tip:Use loadAudioFilesIntoPool for exported projects] To load audio files into a Convolution Reverb or Audio Loop Player in an exported project, use `Engine.loadAudioFilesIntoPool()` rather than accessing the AudioFiles folder directly with `getFolder()`.

## Common Mistakes

- **Pass File object to findFiles**
  **Wrong:** `FileSystem.findFiles(FileSystem.Samples, "*.wav", true)`
  **Right:** `FileSystem.findFiles(FileSystem.getFolder(FileSystem.Samples), "*.wav", true)`
  *`findFiles` requires a `File` object, not a SpecialLocations constant. Use `getFolder()` first to convert the constant to a `File`.*

- **Browse callback is asynchronous**
  **Wrong:** `FileSystem.browse(FileSystem.Desktop, false, "*.wav", function(f){ ... })` and expecting a return value
  **Right:** Use the callback parameter - the selected `File` is passed to the callback asynchronously.
  *All browse methods are async. They return immediately and deliver results via the callback. The callback is not called if the user cancels.*

- **Use correct location type constant**
  **Wrong:** `FileSystem.fromReferenceString("{PROJECT_FOLDER}sound.wav", FileSystem.Desktop)`
  **Right:** `FileSystem.fromReferenceString("{PROJECT_FOLDER}sound.wav", FileSystem.AudioFiles)`
  *Only `AudioFiles`, `Samples`, and `UserPresets` are valid location types for `fromReferenceString`. Other constants trigger a script error.*

- **Store relative paths in presets**
  **Wrong:** Storing absolute file paths in presets or saved state
  **Right:** Store relative paths using `File.toString(1)` and reconstruct with `fromAbsolutePath()` or `getFolder().getChildFile()`
  *Absolute paths break when the plugin is used on a different machine or when the sample folder is relocated.*

- **Cache findFiles results**
  **Wrong:** Calling `findFiles()` on every UI refresh
  **Right:** Cache the file list in a variable or JSON file; rescan only on explicit user action.
  *Directory scanning is expensive, especially on network drives or large content libraries.*
