# FileSystem -- Method Entries

## browse

**Signature:** `undefined browse(var startFolder, Integer forSaving, String wildcard, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Opens a native file dialog via MessageManager::callAsync, involving heap allocation, UI thread dispatch, and blocking OS file chooser.
**Minimal Example:** `FileSystem.browse(FileSystem.Desktop, false, "*.wav", onFileSelected);`

**Description:**
Opens a native file browser dialog for selecting a single file. The dialog runs asynchronously on the message thread and delivers the selected `File` object to the callback. If the user cancels, the callback is not invoked. Only one file dialog can be open at a time -- subsequent calls while a dialog is open are silently ignored due to a static re-entry guard.

When `forSaving` is true, the dialog presents a save-file interface with an overwrite confirmation prompt. When false, it presents an open-file interface. The `wildcard` parameter filters visible files (e.g. `"*.wav"`, `"*.json"`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startFolder | var | no | Initial directory for the dialog. Accepts a `SpecialLocations` constant (int) or a `File` object. | Does not accept absolute path strings -- use `FileSystem.fromAbsolutePath()` first. |
| forSaving | Integer | no | Whether to show a save dialog (true) or open dialog (false). | Boolean value (0 or 1). |
| wildcard | String | no | File type filter for the dialog. | Standard wildcard pattern, e.g. `"*.wav"`, `"*.json"`. |
| callback | Function | no | Function called with the selected `File` object. Not called if user cancels. | Must accept 1 argument. |

**Callback Signature:** callback(file: File)

**Pitfalls:**
- Concurrent browse calls are silently dropped. A static `fileChooserIsOpen` guard prevents multiple file dialogs from opening simultaneously. If you call `browse` while another dialog (from any browse method) is already open, the call returns immediately without error and without invoking the callback.
- [BUG] The `startFolder` parameter does not accept absolute path strings. Unlike `browseForMultipleDirectories` and `browseForMultipleFiles` (which use `getFileFromVar`), this method only checks for integer constants and `File` objects. Passing a string silently results in an empty start directory.

**Cross References:**
- `$API.FileSystem.browseForDirectory$`
- `$API.FileSystem.browseForMultipleFiles$`
- `$API.FileSystem.browseForMultipleDirectories$`
- `$API.FileSystem.getFolder$`
- `$API.FileSystem.fromAbsolutePath$`

**Example:**
```javascript:browse-save-file
// Title: Browse for a file to save
inline function onFileSaved(file)
{
    Console.print("Saved to: " + file.toString(file.FullPath));
};

FileSystem.browse(FileSystem.Desktop, true, "*.json", onFileSaved);
```
```json:testMetadata:browse-save-file
{
  "testable": false,
  "skipReason": "Requires native OS file dialog interaction that cannot be automated."
}
```

## browseForDirectory

**Signature:** `undefined browseForDirectory(var startFolder, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Opens a native directory dialog via MessageManager::callAsync, involving heap allocation, UI thread dispatch, and blocking OS dialog.
**Minimal Example:** `FileSystem.browseForDirectory(FileSystem.Desktop, onDirSelected);`

**Description:**
Opens a native directory browser dialog for selecting a single directory. The dialog runs asynchronously on the message thread and delivers the selected `File` object (pointing to the chosen directory) to the callback. If the user cancels, the callback is not invoked. Subject to the same static re-entry guard as all browse methods -- only one file/directory dialog can be open at a time.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startFolder | var | no | Initial directory for the dialog. Accepts a `SpecialLocations` constant (int) or a `File` object. | Does not accept absolute path strings -- use `FileSystem.fromAbsolutePath()` first. |
| callback | Function | no | Function called with the selected directory as a `File` object. Not called if user cancels. | Must accept 1 argument. |

**Callback Signature:** callback(directory: File)

**Pitfalls:**
- [BUG] The `startFolder` parameter does not accept absolute path strings. Like `browse`, this method only checks for integer constants and `File` objects. Passing a string silently results in an empty start directory. Use `FileSystem.fromAbsolutePath()` to convert a path string to a `File` first.

**Cross References:**
- `$API.FileSystem.browse$`
- `$API.FileSystem.browseForMultipleDirectories$`
- `$API.FileSystem.getFolder$`
- `$API.FileSystem.fromAbsolutePath$`

## browseForMultipleDirectories

**Signature:** `undefined browseForMultipleDirectories(var startFolder, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Opens a native directory dialog via MessageManager::callAsync, involving heap allocation, UI thread dispatch, and blocking OS dialog.
**Minimal Example:** `FileSystem.browseForMultipleDirectories(FileSystem.Desktop, onDirsSelected);`

**Description:**
Opens a native directory browser dialog that allows selecting multiple directories at once. The dialog runs asynchronously on the message thread. The callback receives an `Array` of `File` objects, one for each selected directory. If the user cancels, the callback is not invoked. Subject to the same static re-entry guard as all browse methods.

Unlike `browse` and `browseForDirectory`, this method uses the more permissive `getFileFromVar` utility for resolving the `startFolder` parameter, which also accepts absolute path strings in addition to `SpecialLocations` constants and `File` objects.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startFolder | var | no | Initial directory for the dialog. Accepts a `SpecialLocations` constant (int), a `File` object, or an absolute path string. | -- |
| callback | Function | no | Function called with an `Array` of `File` objects for the selected directories. Not called if user cancels. | Must accept 1 argument. |

**Callback Signature:** callback(directories: Array)

**Pitfalls:**
- The callback receives an `Array` of `File` objects even if only one directory is selected. Always iterate the result rather than using it directly as a single `File`.

**Cross References:**
- `$API.FileSystem.browseForDirectory$`
- `$API.FileSystem.browse$`
- `$API.FileSystem.browseForMultipleFiles$`
- `$API.FileSystem.getFolder$`

## browseForMultipleFiles

**Signature:** `undefined browseForMultipleFiles(var startFolder, String wildcard, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Opens a native file dialog via MessageManager::callAsync, involving heap allocation, UI thread dispatch, and blocking OS dialog.
**Minimal Example:** `FileSystem.browseForMultipleFiles(FileSystem.Desktop, "*.wav", onFilesSelected);`

**Description:**
Opens a native file browser dialog that allows selecting multiple files at once. The dialog runs asynchronously on the message thread. The callback receives an `Array` of `File` objects, one for each selected file. If the user cancels, the callback is not invoked. Subject to the same static re-entry guard as all browse methods.

Like `browseForMultipleDirectories`, this method uses the `getFileFromVar` utility for resolving the `startFolder` parameter, accepting absolute path strings in addition to `SpecialLocations` constants and `File` objects. This method always opens a file-open dialog (never save).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| startFolder | var | no | Initial directory for the dialog. Accepts a `SpecialLocations` constant (int), a `File` object, or an absolute path string. | -- |
| wildcard | String | no | File type filter for the dialog. | Standard wildcard pattern, e.g. `"*.wav"`, `"*.json"`. |
| callback | Function | no | Function called with an `Array` of `File` objects for the selected files. Not called if user cancels. | Must accept 1 argument. |

**Callback Signature:** callback(files: Array)

**Pitfalls:**
- The callback receives an `Array` of `File` objects even if only one file is selected. Always iterate the result rather than using it directly as a single `File`.

**Cross References:**
- `$API.FileSystem.browse$`
- `$API.FileSystem.browseForDirectory$`
- `$API.FileSystem.browseForMultipleDirectories$`
- `$API.FileSystem.getFolder$`

## decryptWithRSA

**Signature:** `String decryptWithRSA(String dataToDecrypt, String publicKey)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Performs BigInteger parsing, RSA key arithmetic, memory block allocation, and string construction -- not audio-thread safe.
**Minimal Example:** `var plainText = FileSystem.decryptWithRSA(encryptedHex, publicKeyString);`

**Description:**
Decrypts a hex-encoded string using an RSA public key and returns the original plaintext string. The input `dataToDecrypt` must be a hexadecimal (base 16) string as produced by `encryptWithRSA`. The method parses the hex string into a `BigInteger`, applies the RSA key operation, and converts the result back to a string.

Returns an empty string in two cases: if the RSA key string is invalid (fails `RSAKey::isValid()`), or if the decrypted data is not valid UTF-8. There is no error message in either case -- the method silently returns empty.

This is a raw RSA operation without hybrid encryption. The maximum data size is limited by the RSA key size.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataToDecrypt | String | no | Hex-encoded encrypted data string (base 16), as produced by `encryptWithRSA`. | Must be a valid hexadecimal string. |
| publicKey | String | no | RSA public key string in JUCE's comma-separated format. | Must pass `RSAKey::isValid()` or the method returns empty string. |

**Pitfalls:**
- Returns an empty string silently when the key is invalid or when the decrypted data is not valid UTF-8. There is no error message or exception to indicate what went wrong, making debugging difficult. Check the return value to detect failure.

**Cross References:**
- `$API.FileSystem.encryptWithRSA$`
- `$API.FileSystem.getSystemId$`

## descriptionOfSizeInBytes

**Signature:** `String descriptionOfSizeInBytes(Integer bytes)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Constructs and returns a String.
**Minimal Example:** `var sizeText = FileSystem.descriptionOfSizeInBytes(1048576);`

**Description:**
Converts a file size in bytes to a human-readable string description. Delegates directly to JUCE's `File::descriptionOfSizeInBytes()`, which returns strings like `"1.5 MB"`, `"200 bytes"`, `"3.2 GB"`, etc. with automatic unit selection based on magnitude.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| bytes | Integer | no | Number of bytes to format as a human-readable size string. | Non-negative integer. |

**Cross References:**
- `$API.FileSystem.getBytesFreeOnVolume$`

## encryptWithRSA

**Signature:** `String encryptWithRSA(String dataToEncrypt, String privateKey)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Performs BigInteger arithmetic, memory block allocation, and string construction -- not audio-thread safe.
**Minimal Example:** `var encrypted = FileSystem.encryptWithRSA("license-data", privateKeyString);`

**Description:**
Encrypts a plaintext string using an RSA private key and returns the result as a hexadecimal (base 16) string. The method converts the input string to a memory block, loads it into a `BigInteger`, applies the RSA key operation, and returns the result as a hex string.

This is a raw RSA operation without hybrid encryption. The maximum data size that can be encrypted is limited by the RSA key size. For typical license key workflows, keep the plaintext short (machine IDs, expiry dates, etc.).

Unlike `decryptWithRSA`, this method does not validate the key before use. If the key string is malformed, the RSA operation produces meaningless output without any error indication.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataToEncrypt | String | no | Plaintext string to encrypt. | Length limited by RSA key size. |
| privateKey | String | no | RSA private key string in JUCE's comma-separated format. | -- |

**Pitfalls:**
- [BUG] No key validation is performed. Unlike `decryptWithRSA` which checks `RSAKey::isValid()`, this method constructs and uses the key unconditionally. A malformed key string produces garbage output without any error.

**Cross References:**
- `$API.FileSystem.decryptWithRSA$`
- `$API.FileSystem.getSystemId$`

**Example:**
```javascript:rsa-encrypt-decrypt-roundtrip
// Title: RSA encryption and decryption roundtrip
// Generate a key pair first using JUCE or another tool.
// These are placeholder key strings for illustration.
var privateKey = "my-rsa-private-key";
var publicKey = "my-rsa-public-key";

var original = "LicenseData:12345";
var encrypted = FileSystem.encryptWithRSA(original, privateKey);
var decrypted = FileSystem.decryptWithRSA(encrypted, publicKey);

Console.print("Encrypted hex: " + encrypted);
Console.print("Decrypted: " + decrypted);
```
```json:testMetadata:rsa-encrypt-decrypt-roundtrip
{
  "testable": false,
  "skipReason": "Requires valid RSA key pair which cannot be generated from HiseScript."
}
```

## findFiles

**Signature:** `Array findFiles(ScriptObject directory, String wildcard, Integer recursive)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem directory traversal with heap allocations (Array, ScriptFile construction) and uses TimeoutExtender. Not audio-thread safe.
**Minimal Example:** `var files = FileSystem.findFiles(FileSystem.getFolder(FileSystem.Desktop), "*.wav", true);`

**Description:**
Returns an `Array` of `File` objects representing all child files and directories within the given directory that match the wildcard pattern. The `directory` parameter must be a `File` object (not a `SpecialLocations` constant) -- use `FileSystem.getFolder()` first to convert a constant to a `File`. If the argument is not a valid `File` object or not a directory, returns an empty array.

The method finds both files and directories, ignores hidden files, and explicitly filters out `.DS_Store` files. Uses a `TimeoutExtender` to prevent script timeout during long directory scans on large directories or network drives.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| directory | ScriptObject | no | The directory to search in. Must be a `File` object pointing to an existing directory. | Must be a `File` object, not a `SpecialLocations` constant. Use `FileSystem.getFolder()` to convert. |
| wildcard | String | no | File matching pattern. | Standard wildcard, e.g. `"*.wav"`, `"*.json"`, `"*"`. |
| recursive | Integer | no | Whether to search subdirectories recursively. | Boolean value (0 or 1). |

**Pitfalls:**
- Does not accept `SpecialLocations` constants directly. Passing an integer constant (e.g. `FileSystem.Desktop`) silently returns an empty array because the `dynamic_cast<ScriptFile*>` check fails for integer values. You must call `FileSystem.getFolder()` first to get a `File` object.

**Cross References:**
- `$API.FileSystem.getFolder$`
- `$API.FileSystem.findFileSystemRoots$`

**Example:**
```javascript:find-wav-files
// Title: Find all WAV files in a directory
var desktopFolder = FileSystem.getFolder(FileSystem.Desktop);
var wavFiles = FileSystem.findFiles(desktopFolder, "*.wav", true);

for (f in wavFiles)
    Console.print(f.toString(f.FullPath));

Console.print("Found " + wavFiles.length + " WAV files");
```
```json:testMetadata:find-wav-files
{
  "testable": false,
  "skipReason": "Depends on user filesystem contents which vary between machines."
}
```

## findFileSystemRoots

**Signature:** `Array findFileSystemRoots()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an Array and creates ScriptFile objects on the heap for each filesystem root. Not audio-thread safe.
**Minimal Example:** `var roots = FileSystem.findFileSystemRoots();`

**Description:**
Returns an `Array` of `File` objects representing all root drives on the current computer. On Windows, this typically returns one entry per mounted drive (e.g. `C:\`, `D:\`). On macOS, this typically returns a single entry (`/`). Delegates to JUCE's `File::findFileSystemRoots()`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.FileSystem.getFolder$`
- `$API.FileSystem.findFiles$`

**Example:**
```javascript:list-filesystem-roots
// Title: List all filesystem root drives
var roots = FileSystem.findFileSystemRoots();

for (r in roots)
    Console.print(r.toString(r.FullPath));

Console.print("Found " + roots.length + " root(s)");
```
```json:testMetadata:list-filesystem-roots
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "roots.length > 0", "value": true}
}
```

## fromAbsolutePath

**Signature:** `ScriptObject fromAbsolutePath(String path)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptFile object on the heap and constructs strings. Not audio-thread safe.
**Minimal Example:** `var f = FileSystem.fromAbsolutePath("C:/Users/MyFile.txt");`

**Description:**
Creates a `File` object from an absolute filesystem path string. The path must be an absolute path (e.g. `C:/Users/MyFile.txt` on Windows or `/Users/name/file.txt` on macOS). If the string is not recognized as an absolute path by JUCE's `File::isAbsolutePath()`, the method returns `undefined`.

The method does not verify that the file or directory actually exists on disk -- it only validates the path format. Use `File.isFile()` or `File.isDirectory()` on the returned object to check existence.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | String | no | Absolute filesystem path to create a File object for. | Must be an absolute path. Relative paths return `undefined`. |

**Pitfalls:**
- Returns `undefined` silently when given a relative path or non-path string. There is no error message -- the method simply returns an empty `var`. Check for `isDefined()` on the result if the path source is untrusted.

**Cross References:**
- `$API.FileSystem.getFolder$`
- `$API.FileSystem.fromReferenceString$`
- `$API.FileSystem.browse$`

## fromReferenceString

**Signature:** `ScriptObject fromReferenceString(String referenceStringOrFullPath, Integer locationType)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptFile object, constructs PoolReference with string operations, and accesses MainController. Not audio-thread safe.
**Minimal Example:** `var f = FileSystem.fromReferenceString("{PROJECT_FOLDER}impulse.wav", FileSystem.AudioFiles);`

**Description:**
Resolves a HISE resource reference string (e.g. `{PROJECT_FOLDER}impulse.wav`) or an absolute file path into a `File` object, using the specified location type to determine the resource pool context.

The method creates a `PoolReference` from the input string and location type. If the reference resolves to an absolute file path, a `File` object is returned directly. If the reference is a valid project-relative reference (not embedded), it is resolved to the actual file on disk. If the reference points to an embedded resource (compiled into the plugin binary), the method returns `undefined` since embedded resources have no file handle.

Only three location types are valid: `FileSystem.AudioFiles`, `FileSystem.Samples`, and `FileSystem.UserPresets`. Passing any other `SpecialLocations` constant triggers a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| referenceStringOrFullPath | String | no | A HISE reference string with `{PROJECT_FOLDER}` wildcard, or an absolute file path. | -- |
| locationType | Integer | no | A `SpecialLocations` constant identifying which resource pool to resolve against. | Must be `FileSystem.AudioFiles`, `FileSystem.Samples`, or `FileSystem.UserPresets`. Others trigger a script error. |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `FileSystem.AudioFiles` | Resolves against the AudioFiles pool directory (impulse responses, loops). |
| `FileSystem.Samples` | Resolves against the Samples pool directory (streaming samples). |
| `FileSystem.UserPresets` | Resolves against the UserPresets directory. |

**Pitfalls:**
- Returns `undefined` silently for embedded references. When a project is compiled, references that were project-relative become embedded in the binary. `fromReferenceString` returns `undefined` for these because no physical file exists. Code that works in the HISE IDE may fail in exported plugins if it depends on this method returning a valid `File` for embedded resources.
- Only three location types are accepted. Using constants like `FileSystem.Desktop`, `FileSystem.AppData`, etc. triggers a script error with the message `"X" is not a valid locationType`. This is because only AudioFiles, Samples, and UserPresets map to HISE's internal `FileHandlerBase::SubDirectories`.

**Cross References:**
- `$API.FileSystem.fromAbsolutePath$`
- `$API.FileSystem.getFolder$`
- `$API.FileSystem.findFiles$`

## getBytesFreeOnVolume

**Signature:** `Integer getBytesFreeOnVolume(var folder)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Calls JUCE's File::getBytesFreeOnVolume() which performs an OS filesystem query. Not audio-thread safe.
**Minimal Example:** `var freeBytes = FileSystem.getBytesFreeOnVolume(FileSystem.Desktop);`

**Description:**
Returns the number of free bytes on the volume containing the specified folder. The `folder` parameter accepts either a `SpecialLocations` constant (int) or a `File` object. Delegates to JUCE's `File::getBytesFreeOnVolume()`.

If the folder parameter is neither a valid `SpecialLocations` constant nor a `File` object, the underlying `juce::File` remains default-constructed (empty), and `getBytesFreeOnVolume()` returns 0.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| folder | var | no | The folder whose volume to query. Accepts a `SpecialLocations` constant (int) or a `File` object. | Does not accept absolute path strings -- use `FileSystem.fromAbsolutePath()` first. |

**Pitfalls:**
- [BUG] Returns 0 silently for invalid folder arguments. If the parameter is not a `SpecialLocations` constant or `File` object (e.g. a string path), the internal File is empty and `getBytesFreeOnVolume()` returns 0 without error. The result is indistinguishable from a genuinely full volume.

**Cross References:**
- `$API.FileSystem.getFolder$`
- `$API.FileSystem.descriptionOfSizeInBytes$`

**Example:**
```javascript:check-free-disk-space
// Title: Check free disk space on the samples volume
var freeBytes = FileSystem.getBytesFreeOnVolume(FileSystem.Samples);
var freeText = FileSystem.descriptionOfSizeInBytes(freeBytes);

Console.print("Free space on samples volume: " + freeText);
```
```json:testMetadata:check-free-disk-space
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "freeBytes > 0", "value": true}
}
```

## getFolder

**Signature:** `ScriptObject getFolder(Integer locationType)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptFile object on the heap, resolves platform-specific paths, and may create directories (AppData). Not audio-thread safe.
**Minimal Example:** `var desktop = FileSystem.getFolder(FileSystem.Desktop);`

**Description:**
Returns a `File` object for the specified special folder location. This is the primary way to obtain a `File` handle for well-known system directories and HISE-managed folders. The `locationType` parameter must be one of the `SpecialLocations` constants defined on the `FileSystem` namespace (e.g. `FileSystem.Desktop`, `FileSystem.Samples`, `FileSystem.AppData`).

The resolution logic differs between backend (HISE IDE) and frontend (compiled plugin) builds for HISE-managed locations (`Samples`, `Expansions`, `AppData`, `UserPresets`, `AudioFiles`). OS-mapped locations (`UserHome`, `Documents`, `Desktop`, `Downloads`, `Applications`, `Temp`, `Music`) resolve the same in all builds.

Returns `undefined` if the resolved path does not point to an existing directory (the `isDirectory()` check fails).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| locationType | Integer | no | A `SpecialLocations` constant identifying which folder to return. | Must be a valid `SpecialLocations` constant (0-11). |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `FileSystem.AudioFiles` | Non-streaming audio files directory. Backend: project AudioFiles subfolder. Frontend: requires `USE_RELATIVE_PATH_FOR_AUDIO_FILES`. |
| `FileSystem.Expansions` | Root folder containing all expansion packs. |
| `FileSystem.Samples` | Streaming sample files. Backend: project Samples subfolder (or current expansion's Samples with FullInstrumentExpansion). Frontend: installed sample location via link files. |
| `FileSystem.UserPresets` | User preset storage. Backend: project UserPresets subfolder. Frontend: app data UserPresets. |
| `FileSystem.AppData` | Application data (Company/Product). Backend: system app data root + settings. Frontend: `FrontendHandler::getAppDataDirectory()`. Creates directory if missing in backend. |
| `FileSystem.UserHome` | User home directory. |
| `FileSystem.Documents` | User documents directory. |
| `FileSystem.Desktop` | User desktop directory. |
| `FileSystem.Downloads` | User downloads directory (constructed as UserHome + "Downloads"). |
| `FileSystem.Applications` | Global applications directory. |
| `FileSystem.Temp` | System temp directory. |
| `FileSystem.Music` | User music directory. |

**Pitfalls:**
- Returns `undefined` if the resolved directory does not exist. For most OS-mapped locations this is unlikely, but for HISE-managed locations (Samples, AudioFiles) in certain build configurations, the directory may not exist. The `Samples` location returns `undefined` when `FullInstrumentExpansion` is active but no expansion is currently loaded.
- [BUG] The integer cast from `var` to `SpecialLocations` has no range check. Passing an integer outside 0-11 results in undefined C++ behavior in the `switch` statement, likely falling through to the `default` case and returning an empty file (then `undefined` from the `isDirectory()` check).

**Cross References:**
- `$API.FileSystem.fromAbsolutePath$`
- `$API.FileSystem.fromReferenceString$`
- `$API.FileSystem.findFiles$`

**Example:**
```javascript:get-special-folders
// Title: Access multiple special folder locations
var desktop = FileSystem.getFolder(FileSystem.Desktop);
var appData = FileSystem.getFolder(FileSystem.AppData);
var samples = FileSystem.getFolder(FileSystem.Samples);

if (isDefined(desktop))
    Console.print("Desktop: " + desktop.toString(desktop.FullPath));

if (isDefined(appData))
    Console.print("AppData: " + appData.toString(appData.FullPath));

if (isDefined(samples))
    Console.print("Samples: " + samples.toString(samples.FullPath));
```
```json:testMetadata:get-special-folders
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "desktop.toString(0).length > 0", "value": true},
    {"type": "REPL", "expression": "appData.toString(0).length > 0", "value": true}
  ]
}
```

## getSystemId

**Signature:** `String getSystemId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a String constructed from JUCE's machine ID utilities.
**Minimal Example:** `var machineId = FileSystem.getSystemId();`

**Description:**
Returns a unique machine identifier string for the current computer. The ID is derived from hardware characteristics via JUCE's `OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs()` and returns the first entry from the list. This is typically used for license key validation, tying a license to a specific machine.

The returned string is deterministic for a given machine -- calling it multiple times returns the same value. The format is a hex string representing the hardware fingerprint.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.FileSystem.encryptWithRSA$`
- `$API.FileSystem.decryptWithRSA$`

## loadExampleAssets

**Signature:** `undefined loadExampleAssets()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Backend-only. Accesses BackendProcessor, creates ExampleAssetManager, and populates it with dummy files. Not audio-thread safe.
**Minimal Example:** `FileSystem.loadExampleAssets();`

**Description:**
Initializes the HISE example asset manager, which provides dummy audio files, MIDI files, and filmstrip images for use in code snippets and examples. This is a backend-only method -- in compiled plugins (frontend builds), the entire method body is compiled out and the call is a no-op.

The method accesses the `BackendProcessor`'s asset manager (lazily created on first access) and calls `initialise()` to populate it with example resources. These resources are used by the snippet evaluation system to provide realistic test data without requiring actual user content.

**Parameters:**

(No parameters.)

**Pitfalls:**
- No-op in compiled plugins. The method body is entirely wrapped in `#if USE_BACKEND`, so calling it in an exported plugin does nothing. No error or warning is produced.

**Cross References:**
- `$API.FileSystem.getFolder$`
