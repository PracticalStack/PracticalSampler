<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# Expansion

A scripting handle to a single installed expansion pack. Each Expansion object provides access to the pack's resource pools, metadata properties, and folder locations.

An Expansion can:

1. **List resources** - enumerate the sample maps, audio files, images, MIDI files, data files, and user presets bundled with the pack.
2. **Build pool references** - construct `{EXP::Name}path` wildcard strings that address expansion resources in any pool-based API.
3. **Read and write data files** - persist per-expansion JSON configuration in the AdditionalSourceCode folder.
4. **Manage the sample folder** - redirect where the expansion looks for its samples on disk.
5. **Rebuild user presets** - extract updated presets from encoded expansion data after an update.

Expansion objects are never created directly. Obtain them from the `ExpansionHandler`:

```javascript
const var expHandler = Engine.createExpansionHandler();
const var e = expHandler.getExpansion("MyExpansion");
// or iterate all:
for (e in expHandler.getExpansionList())
    Console.print(e.getProperties().Name);
```

The resource list methods fall into two groups based on how they discover files:

| Discovery method | Methods | Behaviour |
|------------------|---------|-----------|
| Pool lookup (fast) | `getSampleMapList`, `getMidiFileList`, `getDataFileList` | Returns files already known to the pool from initialisation |
| Filesystem scan (slower first call) | `getAudioFileList`, `getImageList` | Triggers `loadAllFilesFromProjectFolder()` before listing |

> The scripting wrapper holds a weak reference to the underlying expansion object. If the expansion is unloaded (via `unloadExpansion()` or by the handler), the reference becomes invalid and subsequent method calls will throw a script error.

> HISE supports three expansion types: FileBased (0), Intermediate (1), and Encrypted (2). Some methods - notably `rebuildUserPresets` - only work with Intermediate or Encrypted types. Use `getExpansionType()` to check.

## Common Mistakes

- **Wrong:** `e.setSampleFolder("/path/to/samples")`
  **Right:** `e.setSampleFolder(FileSystem.getFolder(FileSystem.Samples))`
  *`setSampleFolder` requires a File object, not a string path. Passing a string silently returns false.*

- **Wrong:** Calling `e.rebuildUserPresets()` on a FileBased expansion
  **Right:** Check the type first: only call on Intermediate or Encrypted expansions
  *`rebuildUserPresets` silently returns false for FileBased expansions because there is no encoded data to extract from.*

- **Wrong:** Using expansion resources without handling the "no expansion" case
  **Right:** Check `isDefined(expansion)` and fall back to `{PROJECT_FOLDER}` references
  *The expansion callback receives `undefined` when no expansion is active. Code that assumes a valid Expansion object will fail.*

- **Wrong:** Building resource lists only from expansions
  **Right:** Include `Sampler.getSampleMapList()` or `Engine.loadAudioFilesIntoPool()` for the project's own embedded content
  *Expansions supplement the project's root resources. A browser that only shows expansion content hides the base product's sample maps and audio files.*
