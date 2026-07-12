# Expansion -- Class Analysis

## Brief
Handle to a single installed expansion pack with access to its resources, properties, and sample data.

## Purpose
Expansion is a scripting handle to one installed expansion pack within the HISE expansion system. It provides read access to the expansion's resource pools (sample maps, audio files, images, MIDI files, data files, user presets), metadata properties (name, version, tags), and folder locations. It also supports loading and writing JSON data files, managing the sample folder location via link files, rebuilding user presets from encoded expansions, and unloading the expansion at runtime. Expansion objects are created by `ExpansionHandler` -- they are never instantiated directly.

## Details

### Resource Pool Access

Each expansion has its own `PoolCollection` inherited from `FileHandlerBase`. The list methods query these pools:

| Method | Pool | Pre-loads Files |
|--------|------|-----------------|
| getSampleMapList | SampleMapPool | No (loaded at init) |
| getImageList | ImagePool | Yes |
| getAudioFileList | AudioSampleBufferPool | Yes |
| getMidiFileList | MidiFilePool | No (loaded at init) |
| getDataFileList | AdditionalDataPool | No |
| getUserPresetList | (filesystem scan) | N/A |

`getImageList` and `getAudioFileList` call `loadAllFilesFromProjectFolder()` before listing, forcing discovery of all files in the expansion's folder. `getSampleMapList` and `getMidiFileList` rely on pools populated during expansion initialisation.

`getUserPresetList` is unique: it scans the filesystem directly rather than using a pool, searching for `.preset` files recursively under the UserPresets subdirectory.

### Return String Formats

All list methods return arrays of strings. The format varies:

- **getSampleMapList**: pool reference string with `.xml` extension stripped
- **getUserPresetList**: relative path from UserPresets folder with `.preset` stripped, backslashes normalized to forward slashes
- **All others**: pool reference string as-is

### Wildcard Reference System

Expansion resources are addressed using wildcard references in the format `{EXP::ExpansionName}relativePath`. See `getWildcardReference()` for construction details.

### loadDataFile Dual Code Path

`loadDataFile` behaves differently based on expansion type -- see `loadDataFile()` for the full dual code path and caching behavior.

### writeDataFile Limitations

`writeDataFile` always writes to the filesystem regardless of expansion type -- see `writeDataFile()` for the pool/filesystem asymmetry with non-FileBased expansions.

### rebuildUserPresets Type Restriction

`rebuildUserPresets` only works with Intermediate or Encrypted expansion types -- see `rebuildUserPresets()` for type restrictions and overwrite behavior.

### WeakReference Lifecycle

The scripting wrapper holds a `WeakReference<Expansion>` to the core expansion object owned by `ExpansionHandler`. If the expansion is unloaded or destroyed, the reference becomes null. Most methods check validity via `objectExists()` before accessing the expansion. After calling `unloadExpansion()`, the reference becomes invalid.

### Properties Object

`getProperties()` returns a dynamic object converted from the expansion's ValueTree -- see `getProperties()` for the full property list and defaults.

## obtainedVia
`ExpansionHandler.getExpansion(expansionName)` or `ExpansionHandler.getExpansionList()`

## minimalObjectToken
e

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `e.setSampleFolder("/path/to/samples")` | `e.setSampleFolder(FileSystem.getFolder(FileSystem.Samples))` | `setSampleFolder` requires a File object, not a string path. Passing a string returns false silently. |
| `e.rebuildUserPresets()` on FileBased expansion | Use only on Intermediate/Encrypted expansions | `rebuildUserPresets` silently returns false for FileBased expansions -- it requires encoded data to extract from. |

## codeExample
```javascript
// Get an expansion from the handler
const var eh = Engine.createExpansionHandler();
const var list = eh.getExpansionList();

if (list.length > 0)
{
    const var e = list[0];
    var props = e.getProperties();
    Console.print("Expansion: " + props.Name + " v" + props.Version);

    // List available sample maps
    var maps = e.getSampleMapList();
    for (m in maps)
        Console.print("  SampleMap: " + m);
}
```

## Alternatives
- `ExpansionHandler` -- Factory that manages the collection of expansions and returns Expansion references. Use ExpansionHandler for discovery, installation, and switching; use Expansion for accessing a specific pack's content.

## Related Preprocessors
None directly. The underlying core class is affected by `HISE_USE_UNLOCKER_FOR_EXPANSIONS` (adds Key property) and `USE_BACKEND` (affects property source resolution), but the scripting wrapper itself has no preprocessor guards.

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Expansion methods have clear return values (empty arrays, false, script errors) for invalid states. No silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics beyond what the WeakReference null checks already provide.
