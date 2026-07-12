# File -- Method Analysis

## copy

**Signature:** `bool copy(ScriptObject target)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (file copy operation).
**Minimal Example:** `var ok = {obj}.copy(targetFile);`

**Description:**
Copies this file to the location specified by the target File object. The target parameter is the full destination file path, not a directory to copy into. Returns `true` if the copy succeeds, `false` otherwise.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| target | ScriptObject | no | Destination file handle (must be a File object) | Must be a File object; reports script error otherwise |

**Pitfalls:**
- After copying, the original File object still references the source path. To work with the copied file, use the target File object.
- The target must be a File object, not a string path. Passing a string reports a script error "target is not a file".

**Cross References:**
- `$API.File.move$`
- `$API.File.rename$`
- `$API.File.copyDirectory$`

## copyDirectory

**Signature:** `bool copyDirectory(ScriptObject target)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs recursive filesystem I/O (directory copy).
**Minimal Example:** `var ok = {obj}.copyDirectory(targetDir);`

**Description:**
Recursively copies this directory to the target location. The target parameter is the actual directory to create, not the parent directory into which the copy should be placed. Returns `true` if the copy succeeds, `false` otherwise. Both the source and target must be File objects. Reports a script error if the target is not a File object or not a directory.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| target | ScriptObject | no | Destination directory handle (must be a File object pointing to a directory) | Must be a File object; target path must be a directory |

**Pitfalls:**
- [BUG] If the target File exists but is not a directory, `reportScriptError("target is not a directory")` is called but execution continues to `copyDirectoryTo()` anyway due to a missing early return. The copy operation proceeds with a non-directory target.
- The target is the destination directory path itself, not a parent to copy into. Passing the parent directory overwrites it with the source content.

**Cross References:**
- `$API.File.copy$`
- `$API.File.move$`

## createDirectory

**Signature:** `ScriptObject createDirectory(String directoryName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (directory creation).
**Minimal Example:** `var dir = {obj}.createDirectory("subFolder");`

**Description:**
Creates a child directory with the given name inside this directory. If the directory already exists, it is not recreated. Returns a File object pointing to the child directory path regardless of whether creation succeeded or the directory already existed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| directoryName | String | no | Name of the subdirectory to create | -- |

**Pitfalls:**
- The method always returns a File handle to the child path even if directory creation fails (e.g., due to permissions). Check `isDirectory()` on the returned File to verify success.
- [BUG] No error is reported if directory creation fails silently (e.g., insufficient permissions). The JUCE `createDirectory()` return value is not checked.

**Cross References:**
- `$API.File.getChildFile$`
- `$API.File.isDirectory$`
- `$API.File.deleteFileOrDirectory$`

## deleteFileOrDirectory

**Signature:** `bool deleteFileOrDirectory()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (deletion).
**Minimal Example:** `var ok = {obj}.deleteFileOrDirectory();`

**Description:**
Deletes the file or directory at this path WITHOUT any confirmation dialog. For directories, deletion is recursive -- all contained files and subdirectories are removed. Symlinks are not followed during recursive deletion. Returns `true` if deletion succeeds, `false` if the file/directory does not exist or deletion fails.

**Parameters:**

(No parameters.)

**Pitfalls:**
- After deletion, the File object still references the deleted path. Calling methods like `isFile()` or `loadAsString()` on it will fail or return empty results. Obtain a new File object if needed.
- Deletion is recursive for directories with no confirmation prompt. There is no undo mechanism.

**Cross References:**
- `$API.File.isFile$`
- `$API.File.isDirectory$`
- `$API.File.createDirectory$`

## extractZipFile

**Signature:** `undefined extractZipFile(var targetDirectory, Integer overwriteFiles, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Dispatches to the Sample Loading Thread via killVoicesAndCall. Kills voices and blocks audio while extraction runs.
**Minimal Example:** `{obj}.extractZipFile(targetDir, true, onExtractProgress);`

**Description:**
Extracts this ZIP archive to the specified target directory. The operation runs asynchronously on the Sample Loading Thread, killing all active voices first. The callback is invoked with a status object at each stage of the extraction (start, per-file or periodic updates, completion). The target directory can be either a File object or an absolute path string.

For archives with fewer than 500 entries, the callback fires for every extracted file. For larger archives, callbacks are throttled to entries larger than 200 MB, with a periodic 200ms timer providing sub-entry progress updates for those large entries. The method also updates the global `SampleManager` preload progress during extraction.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| targetDirectory | var | no | Destination directory for extraction | Must be a File object or an absolute path string |
| overwriteFiles | Integer | no | Whether to overwrite existing files | `true` to overwrite, `false` to skip |
| callback | Function | no | Progress callback receiving a status object | Must accept 1 argument |

**Callback Signature:** callback(data: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Status | Integer | Extraction phase: 0 = starting, 1 = extracting, 2 = complete |
| Progress | Double | Overall progress from 0.0 to 1.0 |
| TotalBytesWritten | Integer | Running total of bytes extracted so far |
| Cancel | Integer | Set to `true` in callback to abort extraction |
| Target | String | Target directory path |
| CurrentFile | String | Filename of the entry currently being extracted |
| Error | String | Error message if extraction fails; empty on success |

**Pitfalls:**
- The method kills all voices before extraction begins. Audio output is silenced during the entire operation.
- User cancellation via setting `Cancel` to `true` in the callback object sets the Error to "User abort" and stops extraction, but already-extracted files remain on disk.
- If the File object is garbage collected during async extraction, the operation aborts silently (checked via weak reference).

**Cross References:**
- `$API.File.getNumZippedItems$`

**Example:**
```javascript:extract-zip-progress
// Title: Extract a ZIP archive with progress tracking
const var zipFile = FileSystem.getFolder(FileSystem.Downloads).getChildFile("samples.zip");
const var targetDir = FileSystem.getFolder(FileSystem.Samples);

inline function onExtract(data)
{
    if (data.Status == 0)
        Console.print("Starting extraction...");
    else if (data.Status == 1)
        Console.print("Extracting: " + data.CurrentFile + " (" + Math.round(data.Progress * 100) + "%)");
    else if (data.Status == 2)
        Console.print("Done! Total bytes: " + data.TotalBytesWritten);

    if (data.Error != "")
        Console.print("Error: " + data.Error);
}

zipFile.extractZipFile(targetDir, true, onExtract);
```
```json:testMetadata:extract-zip-progress
{
  "testable": false,
  "skipReason": "Requires a ZIP file on disk and filesystem side effects"
}
```

## getBytesFreeOnVolume

**Signature:** `Integer getBytesFreeOnVolume()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Queries filesystem volume information (OS I/O call).
**Minimal Example:** `var freeBytes = {obj}.getBytesFreeOnVolume();`

**Description:**
Returns the number of bytes of free disk space on the volume that this file resides on. Delegates directly to `juce::File::getBytesFreeOnVolume()`. Returns 0 if the volume cannot be determined.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.getSize$`
- `$API.File.hasWriteAccess$`

## getChildFile

**Signature:** `ScriptObject getChildFile(String childFileName)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Call Scope Note:** No I/O -- constructs a path object only. Does not check if the file exists.
**Minimal Example:** `var child = {obj}.getChildFile("data.json");`

**Description:**
Returns a new File object representing a child path relative to this directory. The child file does not need to exist on disk -- this method only constructs the path. Supports both simple filenames and relative paths with separators.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| childFileName | String | no | Filename or relative path of the child | -- |

**Cross References:**
- `$API.File.getParentDirectory$`
- `$API.File.createDirectory$`

## getHash

**Signature:** `String getHash()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Reads entire file contents from disk for hashing (I/O plus CPU-intensive SHA-256 computation).
**Minimal Example:** `var hash = {obj}.getHash();`

**Description:**
Reads the file and computes its SHA-256 hash, returning the result as a lowercase hexadecimal string. Uses JUCE's `SHA256` class which reads the entire file into memory for hashing. Useful for verifying file integrity or detecting changes.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Reads the entire file into memory for hashing. For very large files, this may consume significant memory and block the calling thread for a noticeable duration.

**Cross References:**
- `$API.File.getSize$`
- `$API.File.isSameFileAs$`

## getNonExistentSibling

**Signature:** `ScriptObject getNonExistentSibling()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Queries the filesystem to check file existence (I/O).
**Minimal Example:** `var unique = {obj}.getNonExistentSibling();`

**Description:**
Returns a new File object representing a sibling path that does not currently exist on disk. If the current file does not exist, returns a File pointing to the same path. If it does exist, appends a numeric suffix (e.g., `" (2)"`) to find a unique name. Delegates to `juce::File::getNonexistentSibling(false)` -- the `false` parameter means the suffix is appended before the file extension rather than after it.

**Parameters:**

(No parameters.)

**Pitfalls:**
- The returned File object is only guaranteed to be unique at the moment of the call. A concurrent process could create a file at that path before you write to it.

**Cross References:**
- `$API.File.getChildFile$`
- `$API.File.getParentDirectory$`

## getNumZippedItems

**Signature:** `Integer getNumZippedItems()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Reads the ZIP file from disk to parse its central directory (I/O).
**Minimal Example:** `var count = {obj}.getNumZippedItems();`

**Description:**
Returns the number of entries in this ZIP archive. Creates a new `juce::ZipFile` instance from the underlying file on each call (not cached), parses the archive's central directory, and returns `getNumEntries()`. Returns 0 if the file is not a valid ZIP archive or does not exist.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Creates a new ZipFile parser on each call. For repeated queries, store the result in a variable rather than calling this method in a loop.

**Cross References:**
- `$API.File.extractZipFile$`

## getParentDirectory

**Signature:** `ScriptObject getParentDirectory()`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Call Scope Note:** No I/O -- constructs a path object only. Does not check if the directory exists.
**Minimal Example:** `var parent = {obj}.getParentDirectory();`

**Description:**
Returns a new File object representing the parent directory of this file or directory. Delegates directly to `juce::File::getParentDirectory()`. If this file is already at the filesystem root, returns a File pointing to the root path. The parent directory does not need to exist on disk.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.getChildFile$`
- `$API.File.isChildOf$`

## getRedirectedFolder

**Signature:** `ScriptObject getRedirectedFolder()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Reads a link file from disk to resolve the redirect target (I/O).
**Minimal Example:** `var resolved = {obj}.getRedirectedFolder();`

**Description:**
Checks this directory for a HISE link file and returns the redirect target if one exists. HISE uses platform-specific link files (`LinkWindows`, `LinkOSX`, `LinkLinux`) inside directories to redirect folder references to a different location. These are plain-text files containing the absolute path to the redirect target. If no link file is found or the directory is not redirected, returns this same File object. Reports a script error if called on a file (not a directory). If the path does not exist on disk at all (neither file nor directory), returns this same File object without error.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Calling on a file path that exists as a regular file reports a script error. However, if the path does not exist at all, the method returns `this` silently (no error, no redirect check).
- The link file name is platform-specific: `LinkWindows` on Windows, `LinkOSX` on macOS, `LinkLinux` on Linux. Link files from other platforms are ignored.

**Cross References:**
- `$API.FileSystem.getFolder$`

## getRelativePathFrom

**Signature:** `String getRelativePathFrom(ScriptObject otherFile)`
**Return Type:** `String`
**Call Scope:** safe
**Call Scope Note:** No I/O -- computes relative path from two in-memory path strings.
**Minimal Example:** `var rel = {obj}.getRelativePathFrom(baseDir);`

**Description:**
Returns the relative path from the given base directory to this file or directory. The `otherFile` parameter must be a File object pointing to a directory; reports a script error if it is not a directory. Path separators are normalized to forward slashes regardless of the host platform, ensuring cross-platform compatibility in the returned string.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherFile | ScriptObject | no | Base directory to compute the relative path from | Must be a File object pointing to a directory |

**Pitfalls:**
- The base file must be a directory. Passing a file (not a directory) reports a script error "otherFile is not a directory".
- Passing a non-File object (e.g., a string path) reports a script error "otherFile is not a file". Use `FileSystem.fromAbsolutePath()` to create a File object from a string path first.

**Cross References:**
- `$API.File.isChildOf$`
- `$API.File.getParentDirectory$`
- `$API.File.toString$`

## getSize

**Signature:** `Integer getSize()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Queries filesystem metadata (OS I/O call).
**Minimal Example:** `var bytes = {obj}.getSize();`

**Description:**
Returns the size of the file in bytes. Delegates directly to `juce::File::getSize()`. Returns 0 if the file does not exist or is a directory.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.getBytesFreeOnVolume$`
- `$API.File.isFile$`

## hasWriteAccess

**Signature:** `Integer hasWriteAccess()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Queries filesystem permissions (OS I/O call).
**Minimal Example:** `var writable = {obj}.hasWriteAccess();`

**Description:**
Returns `true` if the file or directory has write permissions for the current user. Delegates directly to `juce::File::hasWriteAccess()`. Returns `false` if the file does not exist.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.setReadOnly$`
- `$API.File.isFile$`

## isChildOf

**Signature:** `Integer isChildOf(ScriptObject otherFile, Integer checkSubdirectories)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** No I/O -- compares in-memory path strings only.
**Minimal Example:** `var inside = {obj}.isChildOf(parentDir, true);`

**Description:**
Checks whether this file is a child of the given directory. When `checkSubdirectories` is `true`, checks the entire ancestor chain (any depth). When `false`, only checks if the immediate parent directory matches `otherFile`. Returns `false` silently if `otherFile` is not a valid File object (no error reported).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherFile | ScriptObject | no | The potential parent directory to check against | Must be a File object |
| checkSubdirectories | Integer | no | Whether to check the entire ancestor chain (`true`) or only the immediate parent (`false`) | Boolean |

**Pitfalls:**
- Passing a non-File object as `otherFile` silently returns `false` instead of reporting an error. This can mask bugs where a string path is passed instead of a File object.

**Cross References:**
- `$API.File.getParentDirectory$`
- `$API.File.getRelativePathFrom$`
- `$API.File.isSameFileAs$`

## isDirectory

**Signature:** `bool isDirectory()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Queries the filesystem to check directory status (OS stat call).
**Minimal Example:** `var isDir = {obj}.isDirectory();`

**Description:**
Returns `true` if this path exists on disk and is a directory, `false` otherwise. Delegates to `juce::File::isDirectory()`. Returns `false` for non-existent paths without error.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.isFile$`
- `$API.File.createDirectory$`
- `$API.File.deleteFileOrDirectory$`

## isFile

**Signature:** `bool isFile()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Queries the filesystem to check file existence (OS stat call).
**Minimal Example:** `var exists = {obj}.isFile();`

**Description:**
Returns `true` if this path exists on disk as a regular file (not a directory), `false` otherwise. Internally delegates to `juce::File::existsAsFile()`, which checks both that the path exists and that it is not a directory. Returns `false` for non-existent paths and for directories without error.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.isDirectory$`
- `$API.File.getSize$`
- `$API.File.hasWriteAccess$`

## isSameFileAs

**Signature:** `bool isSameFileAs(ScriptObject otherFile)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** No I/O -- compares in-memory path strings only.
**Minimal Example:** `var same = {obj}.isSameFileAs(otherFile);`

**Description:**
Returns `true` if this file and the given File object reference the same path. Compares the underlying `juce::File` objects using operator `==`, which performs a case-sensitive (on case-sensitive filesystems) or case-insensitive (on Windows/macOS) path string comparison depending on the platform. Does not resolve symlinks or HISE link files before comparing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherFile | ScriptObject | no | File object to compare against | Must be a File object |

**Pitfalls:**
- Passing a non-File object (e.g., a string path) silently returns `false` instead of reporting an error. This matches the pattern of other File methods that accept ScriptObject parameters (`isChildOf`, `move`, `copy`).
- The comparison uses path strings only and does not resolve symlinks or HISE link file redirects. Two File objects pointing to the same physical file via different paths (e.g., one through a symlink) will compare as different.

**Cross References:**
- `$API.File.isChildOf$`
- `$API.File.getRelativePathFrom$`
- `$API.File.getHash$`

## loadAsAudioFile

**Signature:** `var loadAsAudioFile()`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Call Scope Note:** Reads the entire audio file from disk into memory (I/O plus format decoding).
**Minimal Example:** `var audio = {obj}.loadAsAudioFile();`

**Description:**
Reads the audio file and returns its sample data as Buffer objects. The return type depends on the channel count: mono files return a single `Buffer`, while multi-channel files return an `Array` of `Buffer` objects (one per channel). Supports WAV, AIFF, FLAC, OGG, and HLAC formats via `hlac::CompressionHelpers::loadFile`. Reports a script error "No valid audio file" if the file cannot be read or does not exist.

**Parameters:**

(No parameters.)

**Pitfalls:**
- The return type varies by channel count: a single `Buffer` for mono, an `Array` of Buffers for stereo or multi-channel. Code that always indexes the result as an array (e.g., `result[0]`) will fail for mono files. Check with `typeof result == "object"` and `Array.isArray(result)` to distinguish the two cases, or always wrap in an array.
- Reads the entire file into memory. For large audio files, this consumes significant memory. For streaming playback, use the sampler module workflow instead.

**Cross References:**
- `$API.File.writeAudioFile$`
- `$API.File.loadAudioMetadata$`

**Example:**
```javascript:load-audio-handle-channels
// Title: Load audio file and handle mono vs multi-channel
const var audioDir = FileSystem.getFolder(FileSystem.AudioFiles);
const var audioFile = audioDir.getChildFile("recording.wav");
var audio = audioFile.loadAsAudioFile();

// Normalise to array of buffers regardless of channel count
if (!Array.isArray(audio))
    audio = [audio];

Console.print("Channels: " + audio.length);
Console.print("Samples per channel: " + audio[0].length);
```
```json:testMetadata:load-audio-handle-channels
{
  "testable": false,
  "skipReason": "Requires an audio file on disk"
}
```

## loadAsBase64String

**Signature:** `String loadAsBase64String()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Reads the entire file from disk into memory (I/O).
**Minimal Example:** `var b64 = {obj}.loadAsBase64String();`

**Description:**
Reads the file as raw binary data and returns a Base64-encoded string representation. No compression is applied -- the file bytes are Base64-encoded directly. Returns an empty string if the file does not exist or cannot be read. Useful for embedding binary file content in JSON or transmitting file data as text.

**Parameters:**

(No parameters.)

**Pitfalls:**
- No compression is applied despite what the Doxygen comment states. The method simply reads raw bytes and Base64-encodes them. Base64 encoding increases data size by approximately 33%.
- Reads the entire file into memory before encoding. For large files, this can consume significant memory.

**Cross References:**
- `$API.File.loadAsString$`
- `$API.File.getHash$`

## loadAsMidiFile

**Signature:** `JSON loadAsMidiFile(int trackIndex)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Reads the MIDI file from disk and parses its content (I/O plus MIDI parsing).
**Minimal Example:** `var midiData = {obj}.loadAsMidiFile(0);`

**Description:**
Reads a MIDI file and returns a JSON object containing the time signature metadata and all MIDI events from the specified track. Only processes files with the `.mid` extension -- other file types return an empty result without error. The `trackIndex` parameter is zero-based.

The returned object has these properties:

| Property | Type | Description |
|----------|------|-------------|
| `TimeSignature` | JSON | Time signature metadata (NumBars, Nominator, Denominator, Tempo, LoopStart, LoopEnd) |
| `Events` | Array | Array of MessageHolder objects representing the MIDI events |

Uses HISE's internal MIDI resolution of 960 ticks per quarter note. Events are converted using a sample rate of 44100 and a tempo of 120 BPM for timestamp calculation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| trackIndex | Integer | no | Zero-based index of the MIDI track to load | Must be a valid track index in the file |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| TimeSignature | JSON | Time signature and tempo metadata for the MIDI file |
| Events | Array | Array of MessageHolder objects with the track's MIDI events |

**Pitfalls:**
- [BUG] Only processes `.mid` files. Files with other extensions (e.g., `.midi`, `.smf`) are silently ignored, returning an empty value with no error.
- Events are converted with a fixed sample rate of 44100 Hz and tempo of 120 BPM. If the actual playback sample rate or tempo differs, timestamps may not directly correspond to playback positions.

**Cross References:**
- `$API.File.writeMidiFile$`
- `$API.File.loadMidiMetadata$`

**Example:**
```javascript:load-midi-inspect
// Title: Load a MIDI file and inspect its contents
const var midiDir = FileSystem.getFolder(FileSystem.MidiFiles);
const var midiFile = midiDir.getChildFile("pattern.mid");
var midiData = midiFile.loadAsMidiFile(0);

if (isDefined(midiData.TimeSignature))
{
    Console.print("Tempo: " + midiData.TimeSignature.Tempo);
    Console.print("Time Sig: " + midiData.TimeSignature.Nominator + "/" + midiData.TimeSignature.Denominator);
    Console.print("Events: " + midiData.Events.length);
}
```
```json:testMetadata:load-midi-inspect
{
  "testable": false,
  "skipReason": "Requires a MIDI file on disk"
}
```

## loadAsObject

**Signature:** `var loadAsObject()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Reads the file from disk and parses JSON (I/O).
**Minimal Example:** `var data = {obj}.loadAsObject();`

**Description:**
Reads the file as a text string and parses it as JSON, returning the resulting object or array. Reports a script error with the JSON parser error message if the file content is not valid JSON. Internally calls `loadAsString()` followed by `JSON::parse()`.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Reports a script error on parse failure, unlike `loadEncryptedObject` which silently returns undefined. If the file might contain invalid JSON, wrap the call in error handling or validate the file content first.

**Cross References:**
- `$API.File.writeObject$`
- `$API.File.loadAsString$`
- `$API.File.loadEncryptedObject$`
- `$API.File.loadFromXmlFile$`

## loadAsString

**Signature:** `String loadAsString()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Reads the file from disk (I/O).
**Minimal Example:** `var text = {obj}.loadAsString();`

**Description:**
Reads the entire file content as a text string and returns it. Delegates directly to `juce::File::loadFileAsString()`. Returns an empty string if the file does not exist or cannot be read -- no error is reported.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.writeString$`
- `$API.File.loadAsObject$`
- `$API.File.loadAsBase64String$`

## loadAudioMetadata

**Signature:** `JSON loadAudioMetadata()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Reads the file from disk and creates an AudioFormatReader to extract metadata (I/O).
**Minimal Example:** `var meta = {obj}.loadAudioMetadata();`

**Description:**
Reads the audio file's header information without loading the full sample data. Returns a JSON object with properties describing the file format and a nested `Metadata` object containing all format-specific key-value pairs from the audio reader. If the file does not exist or is not a recognized audio format, returns `undefined` silently (no error reported). Supports WAV, AIFF, FLAC, OGG, and other formats registered via `AudioFormatManager::registerBasicFormats()`.

**Parameters:**

(No parameters.)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| SampleRate | Double | The sample rate of the audio file in Hz |
| NumChannels | Integer | Number of audio channels |
| NumSamples | Integer | Total number of sample frames |
| BitDepth | Integer | Bits per sample |
| Format | String | Audio format name (e.g., "WAV", "AIFF", "FLAC") |
| File | String | Full absolute path of the audio file |
| Metadata | JSON | Format-specific metadata key-value pairs (e.g., BWF fields, ID3 tags) |

**Pitfalls:**
- Returns `undefined` silently if the file does not exist or cannot be read as audio. Unlike `loadAsObject`, no script error is reported. Always check the result with `isDefined()` before accessing properties.

**Cross References:**
- `$API.File.loadAsAudioFile$`
- `$API.File.loadMidiMetadata$`

**Example:**
```javascript:audio-metadata-inspect
// Title: Inspect audio file metadata without loading samples
const var audioDir = FileSystem.getFolder(FileSystem.AudioFiles);
const var audioFile = audioDir.getChildFile("recording.wav");
var meta = audioFile.loadAudioMetadata();

if (isDefined(meta))
{
    Console.print("Sample Rate: " + meta.SampleRate);
    Console.print("Channels: " + meta.NumChannels);
    Console.print("Samples: " + meta.NumSamples);
    Console.print("Bit Depth: " + meta.BitDepth);
    Console.print("Format: " + meta.Format);
}
```
```json:testMetadata:audio-metadata-inspect
{
  "testable": false,
  "skipReason": "Requires an audio file on disk"
}
```

## loadEncryptedObject

**Signature:** `var loadEncryptedObject(String key)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Reads the file from disk and performs BlowFish decryption plus JSON parsing (I/O).
**Minimal Example:** `var data = {obj}.loadEncryptedObject("mySecretKey");`

**Description:**
Reads the file as a Base64-encoded string, decrypts it using BlowFish symmetric encryption with the provided key, and parses the decrypted content as JSON. The key must match the one used with `writeEncryptedObject`. The key length is clamped to a maximum of 72 bytes (the BlowFish key size limit). This is NOT RSA encryption -- for public-key encryption, use `FileSystem.encryptWithRSA` / `FileSystem.decryptWithRSA` instead.

If parsing fails after decryption (e.g., wrong key producing garbled output), the method silently returns `undefined` without reporting a script error. This differs from `loadAsObject`, which reports a script error on JSON parse failure.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| key | String | no | BlowFish encryption key | Max 72 bytes; must match the key used for writing |

**Pitfalls:**
- Silently returns `undefined` on decryption or parse failure (wrong key, corrupted file). Unlike `loadAsObject`, no script error is reported. Always check the result with `isDefined()`.
- Uses BlowFish symmetric encryption, not RSA. The key is a shared secret, not a public/private key pair.
- Key length is silently clamped to 72 bytes. Longer keys are truncated without warning, which means two different keys that share the same first 72 bytes will produce identical encryption.

**Cross References:**
- `$API.File.writeEncryptedObject$`
- `$API.File.loadAsObject$`
- `$API.FileSystem.encryptWithRSA$`
- `$API.FileSystem.decryptWithRSA$`

## loadFromXmlFile

**Signature:** `JSON loadFromXmlFile()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Reads the file from disk and performs XML parsing plus ValueTree conversion (I/O).
**Minimal Example:** `var data = {obj}.loadFromXmlFile();`

**Description:**
Reads the file as XML text, parses it into a JUCE ValueTree, and converts the ValueTree into a JSON object using `ValueTreeConverters::convertValueTreeToDynamicObject`. This is the inverse of `writeAsXmlFile`. Returns `undefined` silently if the file does not exist, is empty, or contains invalid XML (no error reported).

The XML-to-JSON conversion preserves the hierarchical structure of the XML document. The root element's tag name is not included as a property -- it becomes the implicit container. Attributes become object properties, and child elements become nested objects or arrays depending on structure.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns `undefined` silently on any parsing failure (missing file, invalid XML, empty content). No script error is reported, unlike `loadAsObject` which reports JSON parse errors. Always check the result with `isDefined()`.
- The round-trip fidelity depends on the `ValueTreeConverters` implementation. Not all JSON structures survive a `writeAsXmlFile` -> `loadFromXmlFile` round-trip identically, particularly arrays of mixed types or deeply nested structures.

**Cross References:**
- `$API.File.writeAsXmlFile$`
- `$API.File.loadAsObject$`
- `$API.File.loadAsString$`

## loadMidiMetadata

**Signature:** `JSON loadMidiMetadata()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Reads the MIDI file from disk and parses its header (I/O plus MIDI parsing).
**Minimal Example:** `var timeSig = {obj}.loadMidiMetadata();`

**Description:**
Reads the MIDI file and returns only the time signature metadata as a JSON object, without loading individual MIDI events. This is a lightweight alternative to `loadAsMidiFile` when you only need tempo and time signature information. Returns `undefined` if the file does not exist or is not a valid MIDI file (no error reported).

The returned object contains these properties:

| Property | Type | Description |
|----------|------|-------------|
| NumBars | Double | Number of bars in the sequence |
| Nominator | Double | Time signature numerator |
| Denominator | Double | Time signature denominator |
| LoopStart | Double | Normalised loop start position (0.0 to 1.0) |
| LoopEnd | Double | Normalised loop end position (0.0 to 1.0) |
| Tempo | Double | Tempo in BPM |

**Parameters:**

(No parameters.)

**Pitfalls:**
- Unlike `loadAsMidiFile`, this method does not check the file extension -- it attempts to parse any file as MIDI. If the file is not a valid MIDI file, it silently returns `undefined`.
- Returns `undefined` silently on failure (no error reported). Always check with `isDefined()` before accessing properties.

**Cross References:**
- `$API.File.loadAsMidiFile$`
- `$API.File.writeMidiFile$`
- `$API.File.loadAudioMetadata$`

## move

**Signature:** `bool move(ScriptObject target)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (file move/rename operation).
**Minimal Example:** `var ok = {obj}.move(targetFile);`

**Description:**
Moves this file to the location specified by the target File object. The target parameter is the full destination file path, not a directory to move into. Returns `true` if the move succeeds, `false` otherwise. Reports a script error if the target is not a valid File object. Delegates to `juce::File::moveFileTo`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| target | ScriptObject | no | Destination file handle (must be a File object) | Must be a File object; reports script error otherwise |

**Pitfalls:**
- After moving, the original File object still references the old path. The internal `juce::File` member is immutable after construction. To work with the file at its new location, use the target File object that was passed as the argument.
- The target must be a File object, not a string path. Passing a string reports a script error "target is not a file".

**Cross References:**
- `$API.File.copy$`
- `$API.File.rename$`

## rename

**Signature:** `bool rename(String newName)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (file rename operation).
**Minimal Example:** `var ok = {obj}.rename("newFileName");`

**Description:**
Renames this file to the given name within the same directory. The original file extension is always preserved -- if `newName` includes an extension, it is replaced with the original file's extension. Internally calls `f.getSiblingFile(newName).withFileExtension(f.getFileExtension())`, then `f.moveFileTo(newFile)`. Returns `true` if the rename succeeds, `false` otherwise.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newName | String | no | New filename (extension is overridden with the original extension) | -- |

**Pitfalls:**
- The original file extension is always preserved regardless of what `newName` contains. Calling `rename("data.txt")` on a `.json` file produces `data.json`, not `data.txt`. To change the extension, use `move()` with a target File object that has the desired extension.
- After renaming, the File object still references the old filename. The internal path is immutable. Obtain a new File handle via `getParentDirectory().getChildFile("newName")` to reference the renamed file.

**Cross References:**
- `$API.File.move$`
- `$API.File.copy$`
- `$API.File.getParentDirectory$`
- `$API.File.getChildFile$`

## setExecutePermission

**Signature:** `bool setExecutePermission(Integer shouldBeExecutable)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Modifies filesystem permissions (OS I/O call).
**Minimal Example:** `var ok = {obj}.setExecutePermission(true);`

**Description:**
Sets or clears the execute permission on this file. Delegates directly to `juce::File::setExecutePermission()`. Returns `true` if the permission change succeeds, `false` otherwise. On Windows, this has no practical effect since Windows does not use Unix-style execute permissions -- executability is determined by file extension. This method is primarily useful on macOS and Linux.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeExecutable | Integer | no | `true` to set execute permission, `false` to remove it | Boolean |

**Cross References:**
- `$API.File.setReadOnly$`
- `$API.File.hasWriteAccess$`

## setReadOnly

**Signature:** `undefined setReadOnly(Integer shouldBeReadOnly, Integer applyRecursively)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies filesystem permissions (OS I/O call). May recurse into subdirectories.
**Minimal Example:** `{obj}.setReadOnly(true, false);`

**Description:**
Sets or clears the read-only (write-protected) attribute on this file or directory. When `applyRecursively` is `true` and this is a directory, the read-only attribute is applied to all files and subdirectories within it. Delegates directly to `juce::File::setReadOnly()`. This is a void method -- it does not return a success/failure indication.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeReadOnly | Integer | no | `true` to make read-only, `false` to make writable | Boolean |
| applyRecursively | Integer | no | `true` to apply to all children of a directory, `false` for this file/directory only | Boolean |

**Pitfalls:**
- No return value is provided to indicate success or failure. Use `hasWriteAccess()` after calling this method to verify the permission change took effect.
- When `applyRecursively` is `true`, this modifies permissions on all files and subdirectories. There is no confirmation prompt or undo mechanism.

**Cross References:**
- `$API.File.hasWriteAccess$`
- `$API.File.setExecutePermission$`

## show

**Signature:** `undefined show()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Dispatches to the message thread via MessageManager::callAsync. Performs OS file-reveal I/O.
**Minimal Example:** `{obj}.show();`

**Description:**
Opens a native file explorer window (Explorer on Windows, Finder on macOS) with this file or directory selected. The reveal operation is dispatched asynchronously to the JUCE message thread via `MessageManager::callAsync`, so the method returns immediately without blocking the calling thread. Delegates to `juce::File::revealToUser()`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.File.isFile$`
- `$API.File.isDirectory$`
- `$API.File.toString$`

## startAsProcess

**Signature:** `bool startAsProcess(String parameters)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Launches an external OS process (I/O, potential blocking).
**Minimal Example:** `var ok = {obj}.startAsProcess("");`

**Description:**
Launches this file as an external process with the given command-line parameters string. Delegates directly to `juce::File::startAsProcess()`. For executables, this starts the program; for document files, it opens them with the default associated application. Returns `true` if the process was successfully launched, `false` otherwise. The parameters string is passed directly to the OS process launch mechanism.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameters | String | no | Command-line parameters to pass to the process | Pass empty string for no parameters |

**Pitfalls:**
- The method only reports whether the process was successfully launched, not whether the process itself completed successfully. The launched process runs independently.
- On macOS, launching `.app` bundles may require the full bundle path. On Windows, file associations determine which application opens a document file.

**Cross References:**
- `$API.File.setExecutePermission$`
- `$API.File.isFile$`
- `$API.File.show$`

## toReferenceString

**Signature:** `String toReferenceString(String folderType)`
**Return Type:** `String`
**Call Scope:** safe
**Call Scope Note:** No I/O -- constructs a pool reference string from in-memory path data.
**Minimal Example:** `var ref = {obj}.toReferenceString("AudioFiles");`

**Description:**
Converts this file's absolute path to a HISE pool reference string relative to the specified project subdirectory. The returned string uses the `{PROJECT_FOLDER}` prefix format used by HISE's resource pool system (e.g., `{PROJECT_FOLDER}sound.wav`). The `folderType` parameter must match one of the HISE project subdirectory identifiers. A trailing `/` is automatically appended to `folderType` if missing. Reports a script error if the folder type is not recognized.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| folderType | String | no | HISE project subdirectory identifier | Must be one of: `AudioFiles`, `Images`, `SampleMaps`, `MidiFiles`, `UserPresets`, `Samples`, `Scripts`, `Binaries`, `Presets`, `XmlPresetBackups`, `AdditionalSourceCode`, `Documentation` |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `"AudioFiles"` | Audio files directory for non-streaming audio resources |
| `"Images"` | Image resources directory |
| `"SampleMaps"` | Sample map XML definitions directory |
| `"MidiFiles"` | MIDI file resources directory |
| `"UserPresets"` | User presets directory |
| `"Samples"` | Streaming sample data directory |
| `"Scripts"` | HiseScript source files directory |
| `"Binaries"` | Compiled binary output directory |
| `"Presets"` | Module preset definitions directory |
| `"XmlPresetBackups"` | XML preset backup directory |
| `"AdditionalSourceCode"` | Additional C++ source code directory |
| `"Documentation"` | Documentation files directory |

**Pitfalls:**
- [BUG] The `DspNetworks` subdirectory cannot be used with this method. The internal identifier for DspNetworks lacks a trailing slash (`"DspNetworks"` instead of `"DspNetworks/"`), but the method auto-appends a slash, producing `"DspNetworks/"` which does not match. This results in an "Illegal folder type" script error.
- The file must actually reside within the specified subdirectory for the reference string to be meaningful. The method does not validate that the file is actually inside the folder -- it constructs the reference regardless.

**Cross References:**
- `$API.FileSystem.fromReferenceString$`
- `$API.File.toString$`
- `$API.File.getRelativePathFrom$`

## toString

**Signature:** `String toString(Integer formatType)`
**Return Type:** `String`
**Call Scope:** safe
**Call Scope Note:** No I/O -- extracts path components from the in-memory file path string.
**Minimal Example:** `var name = {obj}.toString(f.Filename);`

**Description:**
Returns a string representation of the file path according to the specified format constant. The format constants are available as properties on any File object.

| Format Constant | Value | Returns |
|-----------------|-------|---------|
| `FullPath` | 0 | Full absolute path (e.g., `C:/Users/data/file.txt`) |
| `NoExtension` | 1 | Filename without extension (e.g., `file`) |
| `Extension` | 2 | File extension only (e.g., `.txt`) |
| `Filename` | 3 | Filename with extension (e.g., `file.txt`) |

Reports a script error for invalid format values with message "Illegal formatType argument N".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| formatType | Integer | no | Format constant selecting which path component to return | Must be 0-3; use File constants `FullPath`, `NoExtension`, `Extension`, `Filename` |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `FullPath` (0) | Returns the full absolute path including drive/volume, all directories, filename, and extension |
| `NoExtension` (1) | Returns only the filename portion without the file extension or path |
| `Extension` (2) | Returns only the file extension including the leading dot (e.g., `.wav`) |
| `Filename` (3) | Returns the filename with extension but without the directory path |

**Cross References:**
- `$API.File.toReferenceString$`
- `$API.File.getRelativePathFrom$`

**Example:**
```javascript:file-tostring-formats
// Title: Extract different path components from a File
const var dir = FileSystem.getFolder(FileSystem.AudioFiles);
const var audioFile = dir.getChildFile("recording.wav");

// FullPath is machine-dependent, but other formats are deterministic
Console.print(audioFile.toString(audioFile.NoExtension));  // recording
Console.print(audioFile.toString(audioFile.Extension));     // .wav
Console.print(audioFile.toString(audioFile.Filename));      // recording.wav
```
```json:testMetadata:file-tostring-formats
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "audioFile.toString(audioFile.NoExtension)", "value": "recording"},
    {"type": "REPL", "expression": "audioFile.toString(audioFile.Extension)", "value": ".wav"},
    {"type": "REPL", "expression": "audioFile.toString(audioFile.Filename)", "value": "recording.wav"}
  ]
}
```

## writeAsXmlFile

**Signature:** `bool writeAsXmlFile(var jsonData, String tagName)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (file write operation).
**Minimal Example:** `var ok = {obj}.writeAsXmlFile({"key": "value"}, "Settings");`

**Description:**
Converts a JSON object to XML format and writes it to this file. The `tagName` parameter becomes the root XML element name. Internally converts the JSON data to a JUCE ValueTree via `ValueTreeConverters::convertDynamicObjectToValueTree`, then serializes the ValueTree as an XML document string, which is written to disk via `writeString`. Returns `true` if the file was written successfully, `false` otherwise. This is the inverse of `loadFromXmlFile`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | var | no | JSON object to convert and write as XML | Should be a JSON object (DynamicObject) |
| tagName | String | no | Root XML element name | Must be a valid XML tag name (no spaces or special characters) |

**Pitfalls:**
- The conversion goes through JUCE's ValueTree system, which may not preserve all JSON structures identically. Arrays, nested objects, and mixed-type values may be represented differently in the round-trip through `writeAsXmlFile` -> `loadFromXmlFile`. Test round-trip fidelity for complex data structures.
- The `tagName` must be a valid XML identifier. Invalid characters in the tag name (spaces, special characters) may produce malformed XML output.

**Cross References:**
- `$API.File.loadFromXmlFile$`
- `$API.File.writeObject$`
- `$API.File.writeString$`

## writeAudioFile

**Signature:** `bool writeAudioFile(var audioData, Double sampleRate, Integer bitDepth)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (audio file encoding and writing). May allocate significant memory for buffered paths.
**Minimal Example:** `var ok = {obj}.writeAudioFile(buffer, 44100.0, 24);`

**Description:**
Writes audio data to this file in the format determined by the file extension. Supports WAV, AIFF, FLAC, and OGG output formats (registered via `AudioFormatManager::registerBasicFormats()`). The output format is selected by calling `findFormatForFileExtension()` on the file's extension. If the file already exists, it is deleted before writing. Uses a quality parameter of 9 for the audio format writer.

The `audioData` parameter accepts four input shapes:

| Input Shape | Type Check | Channels | Buffering |
|-------------|-----------|----------|-----------|
| Single `Buffer` | `isBuffer()` | Mono | No |
| Array of `Buffer` objects | `isArray()` and `[0].isBuffer()` | Multi-channel (array length) | No |
| Array of number arrays | `isArray()` and `[0].isArray()` | Multi-channel (array length) | Yes |
| Plain number array | `isArray()` and `[0]` is number | Mono | Yes |

For multi-channel data, all channels must have the same number of samples. A size mismatch reports a script error "Size mismatch at channel N". Float values are sanitized via `FloatSanitizers::sanitizeFloatNumber()` for buffered paths to prevent NaN/Inf from being written.

Reports script errors for: writing to a directory, incompatible data types, size mismatch between channels, and unrecognized file extensions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| audioData | var | no | Audio sample data in one of the four accepted shapes | Must be a Buffer, Array of Buffers, Array of number arrays, or plain number array |
| sampleRate | Double | no | Output sample rate in Hz | Positive value (e.g., 44100.0, 48000.0, 96000.0) |
| bitDepth | Integer | no | Bits per sample for the output file | Depends on format; common values: 16, 24, 32 |

**Pitfalls:**
- The output format is determined entirely by the file extension, not by any explicit format parameter. Using `.wav` writes WAV, `.flac` writes FLAC, etc. An unrecognized extension (e.g., `.mp3`) reports a script error.
- HLAC format is NOT supported for writing, only for reading via `loadAsAudioFile`. There is no `.hlac` writer registered.
- For the buffered paths (number arrays), float sanitization is applied per-sample, which may silently clamp NaN/Inf values to zero. The non-buffered Buffer paths do not sanitize.
- [BUG] The existing file is deleted before writing begins. If the write subsequently fails (e.g., invalid bit depth for the format), the original file is lost.

**Cross References:**
- `$API.File.loadAsAudioFile$`
- `$API.File.loadAudioMetadata$`

**Example:**
```javascript:write-stereo-audio
// Title: Write a stereo audio file from two Buffers
const var outputDir = FileSystem.getFolder(FileSystem.AudioFiles);
const var outputFile = outputDir.getChildFile("generated.wav");

// Create two mono buffers (left and right channels)
const var left = Buffer.create(44100);
const var right = Buffer.create(44100);

// Fill with a sine wave (440 Hz)
for (i = 0; i < 44100; i++)
{
    left[i] = Math.sin(2.0 * Math.PI * 440.0 * i / 44100.0);
    right[i] = left[i];
}

var ok = outputFile.writeAudioFile([left, right], 44100.0, 24);
Console.print("Write success: " + ok);
```
```json:testMetadata:write-stereo-audio
{
  "testable": false,
  "skipReason": "Writes to the filesystem and requires cleanup"
}
```

## writeEncryptedObject

**Signature:** `bool writeEncryptedObject(var jsonData, String key)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (encryption and file write operation).
**Minimal Example:** `var ok = {obj}.writeEncryptedObject({"secret": 42}, "myKey123");`

**Description:**
Serializes a JSON object, encrypts it using BlowFish symmetric encryption with the provided key, and writes the result to this file as a Base64-encoded string. The JSON is serialized in compact format (no indentation). The encrypted data is stored as plain Base64 text on disk, not binary. Returns `true` if the file was written successfully, `false` otherwise.

This uses JUCE's `BlowFish` cipher -- a symmetric block cipher. This is NOT the same as RSA public-key encryption provided by `FileSystem.encryptWithRSA` / `FileSystem.decryptWithRSA`. The same key must be used for both encryption and decryption.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | var | no | JSON object to serialize and encrypt | Any JSON-serializable value |
| key | String | no | BlowFish encryption key (shared secret) | Max effective length 72 bytes; longer keys are silently truncated |

**Pitfalls:**
- The key length is silently clamped to 72 bytes (the BlowFish maximum). Keys longer than 72 bytes are truncated without warning, meaning two different keys that share the same first 72 bytes will produce identical encryption.
- Uses BlowFish symmetric encryption, not RSA. Both the writing and reading code must use the same key. For public-key encryption, use `FileSystem.encryptWithRSA` / `FileSystem.decryptWithRSA`.
- The on-disk format is Base64 text, not binary. This is human-readable (though not human-decodable) and will be approximately 33% larger than the encrypted binary data.

**Cross References:**
- `$API.File.loadEncryptedObject$`
- `$API.File.writeObject$`
- `$API.FileSystem.encryptWithRSA$`

## writeMidiFile

**Signature:** `bool writeMidiFile(var eventList, var metadataObject)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (MIDI file encoding and writing). Also allocates HiseMidiSequence and performs event conversion.
**Minimal Example:** `var ok = {obj}.writeMidiFile(events, {"Tempo": 120.0, "Nominator": 4, "Denominator": 4});`

**Description:**
Writes an array of MessageHolder objects as a standard MIDI file (.mid). The `eventList` parameter must be an Array where each element is a MessageHolder object (created via `Engine.createMessageHolder()` or obtained from `loadAsMidiFile`). Non-MessageHolder elements in the array are silently skipped. The `metadataObject` parameter is an optional JSON object specifying time signature and tempo information. If `metadataObject` is not a JSON object (e.g., `undefined` or a number), the defaults are used.

The method creates a `HiseMidiSequence` internally, sets the time signature from the metadata, writes events using HISE's internal resolution of 960 ticks per quarter note, writes to a temporary file, and then moves the temp file to the target path. If the target file already exists, it is deleted before the move.

Returns `true` if the file was written successfully, `false` otherwise. Returns `false` without error if `eventList` is not an Array.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventList | var | no | Array of MessageHolder objects representing MIDI events | Must be an Array; non-MessageHolder elements silently skipped |
| metadataObject | var | no | JSON object with time signature and tempo metadata | Optional; uses defaults if not a JSON object |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| NumBars | Double | Number of bars in the sequence; auto-calculated from last event timestamp if 0 or omitted |
| Nominator | Double | Time signature numerator (default: 4) |
| Denominator | Double | Time signature denominator (default: 4) |
| LoopStart | Double | Normalised loop start position, 0.0 to 1.0 (default: 0.0) |
| LoopEnd | Double | Normalised loop end position, 0.0 to 1.0 (default: 1.0) |
| Tempo | Double | Tempo in BPM (default: 120.0) |

**Pitfalls:**
- [BUG] If `eventList` is not an Array, the method silently returns `false` without reporting an error. No indication is given that the input was invalid.
- [BUG] Non-MessageHolder elements in the event array are silently skipped. If all elements are invalid (e.g., plain objects instead of MessageHolders), an empty MIDI file is written without error.
- The `NumBars` auto-calculation divides the last event's timestamp by `TicksPerQuarter` (960) and rounds up. This only works correctly if event timestamps are in tick units. If events have sample-based timestamps, the auto-calculation produces incorrect bar counts.

**Cross References:**
- `$API.File.loadAsMidiFile$`
- `$API.File.loadMidiMetadata$`
- `$API.Engine.createMessageHolder$`

**Example:**
```javascript:write-midi-file
// Title: Write a simple MIDI pattern to a file
const var midiDir = FileSystem.getFolder(FileSystem.MidiFiles);
const var outputFile = midiDir.getChildFile("pattern.mid");

// Create a simple note sequence
var events = [];

for (i = 0; i < 4; i++)
{
    var noteOn = Engine.createMessageHolder();
    noteOn.setType(noteOn.NoteOn);
    noteOn.setNoteNumber(60 + i * 4);
    noteOn.setVelocity(100);
    noteOn.setTimestamp(i * 960);
    events.push(noteOn);

    var noteOff = Engine.createMessageHolder();
    noteOff.setType(noteOff.NoteOff);
    noteOff.setNoteNumber(60 + i * 4);
    noteOff.setTimestamp(i * 960 + 480);
    events.push(noteOff);
}

var metadata = {
    "Tempo": 120.0,
    "Nominator": 4,
    "Denominator": 4,
    "NumBars": 1
};

var ok = outputFile.writeMidiFile(events, metadata);
Console.print("MIDI write success: " + ok);
```
```json:testMetadata:write-midi-file
{
  "testable": false,
  "skipReason": "Writes to the filesystem and requires cleanup"
}
```

## writeObject

**Signature:** `bool writeObject(JSON jsonData)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (JSON serialization and file write operation).
**Minimal Example:** `var ok = {obj}.writeObject({"name": "preset", "value": 42});`

**Description:**
Serializes a JSON value to a formatted text string and writes it to this file. Internally calls `JSON::toString(jsonData)` to produce the text representation, then delegates to `writeString` to write the result to disk. Returns `true` if the file was written successfully, `false` otherwise. This is the inverse of `loadAsObject`. The output uses JUCE's default JSON formatting (indented, human-readable).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | JSON value to serialize and write | Any value that `JSON::toString` can serialize (objects, arrays, primitives) |

**Cross References:**
- `$API.File.loadAsObject$`
- `$API.File.writeString$`
- `$API.File.writeEncryptedObject$`
- `$API.File.writeAsXmlFile$`

## writeString

**Signature:** `bool writeString(String text)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (file write operation).
**Minimal Example:** `var ok = {obj}.writeString("Hello World");`

**Description:**
Writes the given text string to this file, replacing any existing content. Uses `juce::File::replaceWithText` which writes atomically via a temporary file to prevent data loss on failure. On Linux, explicit LF (`\n`) line endings are forced regardless of the text content. On Windows and macOS, the JUCE default line ending behavior applies. Returns `true` if the file was written successfully, `false` otherwise.

This is the low-level text writing method that other write methods (`writeObject`, `writeAsXmlFile`) delegate to internally.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| text | String | no | Text content to write to the file | -- |

**Cross References:**
- `$API.File.loadAsString$`
- `$API.File.writeObject$`
- `$API.File.writeAsXmlFile$`
- `$API.File.writeEncryptedObject$`
