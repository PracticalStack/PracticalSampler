<!-- Diagram triage:
  - No diagram specs in source JSON; none to render
-->

# File

File represents a handle to a specific file or directory on the local filesystem. It provides methods for reading and writing data in several formats:

1. **Plain text** - raw string content (`writeString` / `loadAsString`)
2. **JSON** - structured data as human-readable JSON (`writeObject` / `loadAsObject`)
3. **Encrypted JSON** - BlowFish-encrypted JSON stored as Base64 (`writeEncryptedObject` / `loadEncryptedObject`)
4. **XML** - data round-tripped through JUCE ValueTree (`writeAsXmlFile` / `loadFromXmlFile`)
5. **Audio** - WAV, AIFF, FLAC, OGG (plus HLAC for reading) (`writeAudioFile` / `loadAsAudioFile`)
6. **MIDI** - standard MIDI files with time signature metadata (`writeMidiFile` / `loadAsMidiFile`)
7. **Base64** - raw binary encoded as a Base64 string (`loadAsBase64String`, read-only)

Beyond I/O, the class supports filesystem operations such as copying, moving, renaming, deleting, directory creation, ZIP extraction, and path inspection. File objects are obtained from `FileSystem` methods or from other File methods like `getChildFile` and `getParentDirectory`:

```js
const var f = FileSystem.getFolder(FileSystem.AppData).getChildFile("settings.json");
```

The `toString` method accepts a format constant to extract different path components:

| Constant | Value | Returns |
|----------|-------|---------|
| `FullPath` | 0 | Full absolute path |
| `NoExtension` | 1 | Filename without extension |
| `Extension` | 2 | File extension only (including the dot) |
| `Filename` | 3 | Filename with extension |

> [!Tip:File path is immutable after creation] The underlying file path is immutable. After calling `rename`, `move`, or `deleteFileOrDirectory`, the File object still references the **original** path. To work with the new location, obtain a fresh File handle.

## Common Mistakes

- **File reference stale after rename or move**
  **Wrong:** `f.rename("newName"); f.toString(0);`
  **Right:** `var newFile = f.getParentDirectory().getChildFile("newName"); f.rename("newName");`
  *After `rename`, `move`, or `deleteFileOrDirectory`, the File object still points to the old path. Obtain a new handle for the updated location.*

- **Use BlowFish key not RSA key**
  **Wrong:** `f.loadEncryptedObject(rsaKey)`
  **Right:** `f.loadEncryptedObject(blowfishKey)`
  *`writeEncryptedObject` / `loadEncryptedObject` use BlowFish symmetric encryption (max 72-byte key), not RSA. For public-key encryption, use `FileSystem.encryptWithRSA` / `FileSystem.decryptWithRSA`.*

- **Guard loadAsObject with fallback**
  **Wrong:** `var data = f.loadAsObject();` then immediately access `data.SomeKey`
  **Right:** `var data = f.loadAsObject(); if (!isDefined(data)) data = {};`
  *On first launch the file may not exist, and `loadAsObject` returns undefined. Always provide a fallback.*

- **Use relative paths for portability**
  **Wrong:** Using `toString(0)` as a portable key across machines
  **Right:** Use `getRelativePathFrom(baseDir)` or `toReferenceString(folderType)`
  *Full paths are machine-specific. Relative paths or pool references are portable across installations.*
