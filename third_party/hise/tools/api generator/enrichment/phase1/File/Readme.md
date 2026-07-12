# File -- Class Analysis

## Brief
File system handle for reading, writing, and manipulating files and directories on disk.

## Purpose
The `File` class represents a handle to a specific file or directory on the local filesystem. It wraps `juce::File` and provides methods for reading and writing text, JSON, encrypted data, audio files, MIDI files, XML, and Base64 content. It also supports filesystem operations such as copying, moving, renaming, deleting, directory creation, ZIP extraction, and path inspection. File objects are never created directly -- they are obtained from `FileSystem` methods (such as `getFolder`, `browse`, or `fromAbsolutePath`) or from other File methods like `getChildFile` and `getParentDirectory`.

## Details

### Immutable Path Handle

The underlying file path is set at construction time and never updated. After calling `rename()`, `move()`, or `deleteFileOrDirectory()`, the File object still references the **original** path. To work with the new location, obtain a fresh File object.

### Serialization Formats

| Format | Write Method | Read Method | Notes |
|--------|-------------|-------------|-------|
| Plain text | `writeString` | `loadAsString` | Linux forces LF line endings |
| JSON | `writeObject` | `loadAsObject` | Reports script error on parse failure |
| Encrypted JSON | `writeEncryptedObject` | `loadEncryptedObject` | BlowFish symmetric cipher (NOT RSA), key max 72 bytes, stored as Base64 |
| XML | `writeAsXmlFile` | `loadFromXmlFile` | Round-trips through JUCE ValueTree; tagName becomes root element |
| Audio | `writeAudioFile` | `loadAsAudioFile` | Supports WAV, AIFF, FLAC, OGG; load also supports HLAC |
| MIDI | `writeMidiFile` | `loadAsMidiFile` | Uses HiseMidiSequence with 960 ticks/quarter resolution |
| Base64 | -- | `loadAsBase64String` | Raw binary to Base64 (no compression despite Doxygen claim) |

### Audio File I/O

See `writeAudioFile` for the four accepted input shapes and format selection by file extension. See `loadAsAudioFile` for return type handling (single Buffer for mono, Array of Buffers for multi-channel).

### MIDI File I/O

See `writeMidiFile` for the metadata object schema (NumBars, Nominator, Denominator, LoopStart, LoopEnd, Tempo). See `loadAsMidiFile` for the returned object structure (TimeSignature and Events properties) and the `.mid` extension requirement.

### HISE Link File Redirection

`getRedirectedFolder()` checks for platform-specific link files (`LinkWindows`, `LinkOSX`, `LinkLinux`) inside a directory. These plain-text files contain the absolute path to a redirect target. This is the same mechanism HISE uses for sample folder redirection.

### ZIP Extraction

See `extractZipFile` for the full callback status object schema and throttling behavior for large archives. The operation runs on the Sample Loading Thread via `killVoicesAndCall`.

### Pool Reference Strings

See `toReferenceString` for the full list of valid `folderType` identifiers and the `{PROJECT_FOLDER}` reference string format. See `FileSystem.fromReferenceString` for the reverse conversion.

## obtainedVia
`FileSystem.getFolder(location)` -- or other FileSystem factory methods (`browse`, `fromAbsolutePath`, `fromReferenceString`, `findFiles`).

## minimalObjectToken
f

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| `FullPath` | 0 | int | Full absolute path string | Format |
| `NoExtension` | 1 | int | Filename without extension | Format |
| `Extension` | 2 | int | File extension only | Format |
| `Filename` | 3 | int | Filename with extension | Format |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `f.rename("newName"); f.toString(0);` | `var newFile = f.getParentDirectory().getChildFile("newName"); f.rename("newName");` | After `rename`/`move`/`delete`, the File object still points to the old path. Obtain a new File handle for the updated location. |
| `f.loadEncryptedObject(rsaKey)` | `f.loadEncryptedObject(blowfishKey)` | `writeEncryptedObject`/`loadEncryptedObject` use BlowFish symmetric encryption (max 72-byte key), not RSA. For RSA, use `FileSystem.encryptWithRSA`. |

## codeExample
```javascript
// Get a file handle and perform basic I/O
var f = FileSystem.getFolder(FileSystem.Documents).getChildFile("data.json");
f.writeObject({ "name": "test", "value": 42 });
var data = f.loadAsObject();
```

## Alternatives
- `FileSystem` -- provides folder locations and file browsing; returns File handles
- `Buffer` -- for in-memory audio sample data (see `Engine.loadAudioFileIntoBufferArray`)

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- File.toReferenceString -- value-check (logged)
