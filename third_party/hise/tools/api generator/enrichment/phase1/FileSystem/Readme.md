# FileSystem -- Class Analysis

## Brief
Namespace for accessing special folder locations, file browsing dialogs, and RSA encryption.

## Purpose
FileSystem is a namespace-style API class (not instantiable) available as the global `FileSystem` object in all HiseScript contexts. It serves as the primary factory for `File` objects by resolving special folder constants, absolute paths, and HISE reference strings into file handles. It also provides native file browser dialogs (single and multi-select, for files and directories), RSA encryption/decryption for license key workflows, and volume/system queries. The class bridges HiseScript to the underlying platform filesystem, abstracting the differences between backend (IDE) and frontend (compiled plugin) directory structures.

## Details

### Special Location Resolution

The core mechanism is `getFileStatic()`, which resolves a `SpecialLocations` enum value to a `juce::File`. The resolution differs significantly between backend and frontend builds:

**HISE-managed locations (backend/frontend differ):**

| Constant | Backend (IDE) | Frontend (Plugin) |
|----------|---------------|-------------------|
| `Samples` | Project's Samples subfolder (or current expansion's Samples if `FullInstrumentExpansion` is active) | Installed sample location via link files |
| `Expansions` | Expansion handler's root folder | Same |
| `AppData` | `ProjectHandler::getAppDataRoot()` + Company/Product from settings | `FrontendHandler::getAppDataDirectory()` |
| `UserPresets` | Project's UserPresets subfolder | App data UserPresets subfolder |
| `AudioFiles` | Project's AudioFiles subfolder | Requires `USE_RELATIVE_PATH_FOR_AUDIO_FILES` flag |

**OS-mapped locations (same in all builds):**

| Constant | Resolution |
|----------|-----------|
| `UserHome` | JUCE `userHomeDirectory` |
| `Documents` | JUCE `userDocumentsDirectory` |
| `Desktop` | JUCE `userDesktopDirectory` |
| `Downloads` | UserHome + "Downloads" (manually constructed, not a JUCE special location) |
| `Applications` | JUCE `globalApplicationsDirectory` |
| `Temp` | JUCE `tempDirectory` |
| `Music` | JUCE `userMusicDirectory` |

### File Browser Dialogs

All four browse methods share a common internal architecture: async execution via `MessageManager::callAsync()`, a static re-entry guard preventing multiple simultaneous dialogs, and callback-based result delivery (no return value; callback is not invoked on cancel). See `browse()`, `browseForDirectory()`, `browseForMultipleDirectories()`, and `browseForMultipleFiles()` for per-method details and parameter differences.

### PoolReference Integration

See `fromReferenceString()` for the full pool reference resolution API. Only `AudioFiles`, `Samples`, and `UserPresets` are valid location types for reference strings.

### RSA Encryption

See `encryptWithRSA()` and `decryptWithRSA()` for the raw RSA operations. Data is hex-encoded (base 16) and limited by the RSA key size (no hybrid encryption). `getSystemId()` provides the machine identifier typically used alongside these methods for license key workflows.

## obtainedVia
Global namespace -- always available as `FileSystem` in any HiseScript context.

## minimalObjectToken


## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| `AudioFiles` | 0 | int | Non-streaming audio files directory (impulse responses, loops) | SpecialLocations |
| `Expansions` | 1 | int | Expansion pack root folder | SpecialLocations |
| `Samples` | 2 | int | Sample files used by the streaming engine | SpecialLocations |
| `UserPresets` | 3 | int | User preset storage directory | SpecialLocations |
| `AppData` | 4 | int | Application data directory (Company/Product) | SpecialLocations |
| `UserHome` | 5 | int | User home directory | SpecialLocations |
| `Documents` | 6 | int | User documents directory | SpecialLocations |
| `Desktop` | 7 | int | User desktop directory | SpecialLocations |
| `Downloads` | 8 | int | User downloads directory | SpecialLocations |
| `Applications` | 9 | int | Global applications directory | SpecialLocations |
| `Temp` | 10 | int | System temp directory | SpecialLocations |
| `Music` | 11 | int | User music directory | SpecialLocations |

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `FileSystem.browse(FileSystem.Desktop, false, "*.wav", function(f){ ... })` expecting a return value | Use the callback parameter -- the `File` is passed to the callback function asynchronously | Browse methods are async and deliver results via callback, not return value. The callback is not called if the user cancels. |
| `FileSystem.fromReferenceString("{PROJECT_FOLDER}sound.wav", FileSystem.Desktop)` | `FileSystem.fromReferenceString("{PROJECT_FOLDER}sound.wav", FileSystem.AudioFiles)` | Only `AudioFiles`, `Samples`, and `UserPresets` are valid location types for `fromReferenceString`. Other constants trigger a script error. |
| `FileSystem.findFiles(FileSystem.Samples, "*.wav", true)` | `FileSystem.findFiles(FileSystem.getFolder(FileSystem.Samples), "*.wav", true)` | `findFiles` requires a `File` object as its directory parameter, not a SpecialLocations constant. Use `getFolder()` first to get the File object. |

## codeExample
```javascript
// Get a reference to the user's desktop folder
var desktop = FileSystem.getFolder(FileSystem.Desktop);

// List all WAV files recursively
var wavFiles = FileSystem.findFiles(desktop, "*.wav", true);

// Browse for a file asynchronously
FileSystem.browse(FileSystem.Desktop, false, "*.wav", function(f)
{
    Console.print(f.toString(f.FullPath));
});
```

## Alternatives
- `File` -- FileSystem returns File objects from `getFolder`/`fromAbsolutePath`/`browse`; File is the handle you operate on after obtaining it from FileSystem.

## Related Preprocessors
- `USE_BACKEND` -- Controls directory resolution for AppData, UserPresets, AudioFiles, and enables `loadExampleAssets`.
- `USE_RELATIVE_PATH_FOR_AUDIO_FILES` -- Controls whether AudioFiles location is available in frontend builds (defaults to enabled).

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: FileSystem is a stateless namespace with no timeline dependencies or deferred initialization patterns. Methods either succeed immediately or report errors synchronously (except browse, which is inherently async by design). No parse-time diagnostics would catch meaningful issues.
