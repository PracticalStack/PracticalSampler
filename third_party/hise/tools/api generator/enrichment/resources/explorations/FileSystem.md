# FileSystem -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisite table (FileSystem -> File)
- `enrichment/resources/survey/class_survey_data.json` -- FileSystem entry (creates File, seeAlso File)
- `enrichment/base/FileSystem.json` -- 15 API methods
- No prerequisite Readme loaded (File has not been enriched yet)
- No existing base class exploration applies

## Source Locations

- **Header:** `HISE/hi_scripting/scripting/api/ScriptingApi.h` lines 1761-1858
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 7394-7848
- **ScriptFile (File wrapper):** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 324-486
- **Registration:** `HISE/hi_scripting/scripting/ScriptProcessorModules.cpp` (5 registrations)

---

## Class Declaration

```cpp
class FileSystem : public ApiClass,
                   public ScriptingObject,
                   public ControlledObject
```

**Inheritance chain:**
- `ApiClass` -- provides `addConstant()` / `ADD_API_METHOD_N` registration pattern for namespace-style classes
- `ScriptingObject` -- provides `getScriptProcessor()`, `reportScriptError()`
- `ControlledObject` -- provides `getMainController()` for accessing core HISE infrastructure

**Category:** namespace (not an instantiable object). Accessed as global `FileSystem` in scripts.

**Object name:** `"FileSystem"` (returned by `getObjectName()`)

---

## Constructor -- Constants and Method Registration

```cpp
ScriptingApi::FileSystem::FileSystem(ProcessorWithScriptingContent* pwsc):
    ApiClass((int)numSpecialLocations),  // reserves space for 12 constants
    ScriptingObject(pwsc),
    ControlledObject(pwsc->getMainController_()),
    p(pwsc)
```

### Constants (addConstant calls)

All constants are `int` values corresponding to the `SpecialLocations` enum:

| Name | Value | Enum Member | Description |
|------|-------|-------------|-------------|
| `Samples` | 2 | `Samples` | Sample files used by streaming engine |
| `Expansions` | 1 | `Expansions` | Expansion pack root folder |
| `AudioFiles` | 0 | `AudioFiles` | Non-streaming audio files (impulse responses, loops) |
| `UserPresets` | 3 | `UserPresets` | User preset storage directory |
| `AppData` | 4 | `AppData` | App data directory (Company/Product) |
| `UserHome` | 5 | `UserHome` | User home directory |
| `Documents` | 6 | `Documents` | User documents directory |
| `Desktop` | 7 | `Desktop` | User desktop directory |
| `Downloads` | 8 | `Downloads` | User downloads directory |
| `Applications` | 9 | `Applications` | Global applications directory |
| `Temp` | 10 | `Temp` | System temp directory |
| `Music` | 11 | `Music` | User music directory |

The enum `numSpecialLocations` = 12 is used as the ApiClass capacity argument.

### Method Registrations

All methods use plain `ADD_API_METHOD_N` (not typed variants):

```cpp
ADD_API_METHOD_1(getFolder);
ADD_API_METHOD_3(findFiles);
ADD_API_METHOD_0(getSystemId);
ADD_API_METHOD_1(descriptionOfSizeInBytes);
ADD_API_METHOD_4(browse);
ADD_API_METHOD_2(browseForDirectory);
ADD_API_METHOD_1(fromAbsolutePath);
ADD_API_METHOD_2(fromReferenceString);
ADD_API_METHOD_1(getBytesFreeOnVolume);
ADD_API_METHOD_2(encryptWithRSA);
ADD_API_METHOD_2(decryptWithRSA);
ADD_API_METHOD_0(findFileSystemRoots);
ADD_API_METHOD_0(loadExampleAssets);
ADD_API_METHOD_2(browseForMultipleDirectories);
ADD_API_METHOD_3(browseForMultipleFiles);
```

**Wrapper struct** uses `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` (no typed wrappers).

---

## SpecialLocations Enum

```cpp
enum SpecialLocations
{
    AudioFiles,      // 0
    Expansions,      // 1
    Samples,         // 2
    UserPresets,     // 3
    AppData,         // 4
    UserHome,        // 5
    Documents,       // 6
    Desktop,         // 7
    Downloads,       // 8
    Applications,    // 9
    Temp,            // 10
    Music,           // 11
    numSpecialLocations  // 12
};
```

---

## SpecialLocations Behavioral Tracing (getFileStatic)

The `getFileStatic(SpecialLocations l, MainController* mc)` method is the core resolution function. Each location resolves differently depending on the build target:

### HISE-managed locations (backend/frontend differ)

**Samples (2):**
- If `FullInstrumentExpansion::isEnabled(mc)` AND a current expansion exists: uses `expansion->getSubDirectory(FileHandlerBase::Samples)`
- Otherwise: uses `mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples)`
- In backend: resolves to the project's Samples subfolder
- In frontend: resolves to the installed sample location (via link files)

**Expansions (1):**
- Always: `mc->getExpansionHandler().getExpansionFolder()`
- Returns the root folder containing all expansion packs

**AppData (4):**
- Backend (`#if USE_BACKEND`): `ProjectHandler::getAppDataRoot(mc)` then appends `Company/Product` from HiseSettings. Creates directory if missing.
- Frontend (`#else`): `FrontendHandler::getAppDataDirectory()`
- Resolves to platform-specific app data: `Users/Library/Application Support/Company/Product/` (macOS) or `Users/AppData/Local/Company/Product` (Windows)

**UserPresets (3):**
- Backend: `mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::UserPresets)` -- project's UserPresets folder
- Frontend: `FrontendHandler::getUserPresetDirectory()` -- app data UserPresets subfolder

**AudioFiles (0):**
- Backend: `mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::AudioFiles)` -- project's AudioFiles folder
- Frontend: Requires `USE_RELATIVE_PATH_FOR_AUDIO_FILES` flag (defaults to 1 in LibConfig.h). Uses `FrontendHandler::getAdditionalAudioFilesDirectory()`
- NOTE: Missing `break` statement before `default:` -- falls through to default after setting `f`. This is likely intentional (the default is just `break`).

### OS-mapped locations (platform-independent logic)

**UserHome (5):** `File::getSpecialLocation(File::userHomeDirectory)`
**Documents (6):** `File::getSpecialLocation(File::userDocumentsDirectory)`
**Desktop (7):** `File::getSpecialLocation(File::userDesktopDirectory)`
**Music (11):** `File::getSpecialLocation(File::userMusicDirectory)`
**Downloads (8):** `File::getSpecialLocation(File::userHomeDirectory).getChildFile("Downloads")` -- note: not a JUCE special location, manually constructed
**Applications (9):** `File::getSpecialLocation(File::globalApplicationsDirectory)`
**Temp (10):** `File::getSpecialLocation(File::tempDirectory)`

---

## ScriptFile (File wrapper class) -- Context for FileSystem

FileSystem creates `ScriptingObjects::ScriptFile` instances. Key facts about ScriptFile:

- **Class:** `ScriptingObjects::ScriptFile : public ConstScriptingObject`
- **Object name:** `"File"` (returned to HiseScript as type "File")
- **Public member:** `File f` -- the underlying `juce::File`
- **Constructor:** `ScriptFile(ProcessorWithScriptingContent* p, const File& f_)`
- **Debug value:** `f.getFullPathName()` (shows full path in debugger)
- **Double-click:** `f.revealToUser()` (opens in Explorer/Finder from backend)

ScriptFile has its own extensive API (30+ methods): file I/O, directory operations, audio/MIDI file handling, zip extraction, encryption, etc. This is the `File` class in the scripting API.

---

## getFileFromVar -- Static Utility

The `getFileFromVar` static method is a critical utility used by FileSystem itself and by other classes (e.g., `ScriptingApiObjects`). It resolves a `var` argument to a `juce::File` through multiple strategies:

```cpp
static File getFileFromVar(const var& fileObjectDirectoryConstantOrAbsolutePath, MainController* mc);
```

**Resolution order:**
1. If `void` or `undefined` -> return empty `File()`
2. If `isInt()` -> cast to `SpecialLocations`, call `getFileStatic(constant, mc)`
3. If `ScriptFile*` -> extract `sf->f`
4. If `isAbsolutePath(toString())` -> create `File` from string
5. Otherwise -> return empty `File()`

This means many FileSystem methods accept either a `SpecialLocations` constant (int), a `File` object, or an absolute path string as their folder parameter.

---

## browseInternally -- File Dialog Infrastructure

All browse methods delegate to `browseInternally`:

```cpp
void browseInternally(File f, bool forSaving, bool isDirectory, String wildcard, var callback, bool multiple);
```

**Key patterns:**

1. **Static guard against re-entry:** Uses `static bool fileChooserIsOpen` to prevent opening multiple file dialogs simultaneously. If one is already open, the method returns immediately (silently).

2. **WeakCallbackHolder:** Wraps the HiseScript callback in a `WeakCallbackHolder` with 1 argument. Sets high priority and increments reference count for safe async use.

3. **Async execution:** The entire file chooser runs via `MessageManager::callAsync(cb)` -- the lambda captures all state and runs on the message thread.

4. **Callback invocation:** The callback receives:
   - For single-file/directory selection: a single `ScriptFile` object
   - For multiple selection: an `Array<var>` of `ScriptFile` objects
   - If user cancels: callback is NOT called (only called `if (a.isObject())`)

5. **File dialog types used:**
   - `fc.browseForDirectory()` -- single directory
   - `fc.browseForMultipleDirectories()` -- multiple directories
   - `fc.browseForFileToSave(true)` -- file save (with overwrite warning)
   - `fc.browseForFileToOpen()` -- single file open
   - `fc.browseForMultipleFilesToOpen()` -- multiple file open

### Method-to-browseInternally mapping

| Method | forSaving | isDirectory | wildcard | multiple |
|--------|-----------|-------------|----------|----------|
| `browse(folder, forSaving, wildcard, cb)` | param | false | param | false |
| `browseForDirectory(folder, cb)` | false | true | "" | false |
| `browseForMultipleDirectories(folder, cb)` | false | true | "" | true |
| `browseForMultipleFiles(folder, wildcard, cb)` | false | false | param | true |

### startFolder resolution differences

- `browse` and `browseForDirectory`: Manually check `startFolder.isInt()` then `dynamic_cast<ScriptFile*>`
- `browseForMultipleDirectories` and `browseForMultipleFiles`: Use the `getFileFromVar` static utility
- This means `browse`/`browseForDirectory` do NOT accept absolute path strings as startFolder, while the multiple-selection variants DO (via getFileFromVar)

---

## fromReferenceString -- PoolReference System

This method bridges the FileSystem to HISE's resource pool/reference system:

```cpp
var fromReferenceString(String referenceStringOrFullPath, var locationType)
{
    auto sub = getSubdirectory(locationType);
    PoolReference ref(getScriptProcessor()->getMainController_(), referenceStringOrFullPath, sub);

    if(ref.isAbsoluteFile())
        return var(new ScriptFile(getScriptProcessor(), File(referenceStringOrFullPath)));

    if ((ref.isValid()) && !ref.isEmbeddedReference())
    {
        auto f = ref.getFile();
        return var(new ScriptFile(getScriptProcessor(), File(f)));
    }

    return {};
}
```

**PoolReference** (`PoolHelpers::Reference`): HISE's resource reference system that resolves strings like `{PROJECT_FOLDER}file.wav` to actual files. Modes include:
- `Invalid` -- unresolvable
- `AbsolutePath` -- full OS path
- `ExpansionPath` -- within an expansion pack
- `ProjectPath` -- relative to project folder (becomes EmbeddedResource at export)
- `EmbeddedResource` -- compiled into plugin binary

**getSubdirectory** maps SpecialLocations to `FileHandlerBase::SubDirectories`:
- `AudioFiles` -> `FileHandlerBase::AudioFiles`
- `Samples` -> `FileHandlerBase::Samples`
- `UserPresets` -> `FileHandlerBase::UserPresets`
- All others -> `reportScriptError()` (only these three are valid for reference strings)

**Important:** Returns empty `var` (undefined) if the reference is embedded (compiled) -- you can't get a File handle to embedded resources.

---

## findFiles -- Directory Listing

```cpp
var findFiles(var directory, String wildcard, bool recursive)
```

- Casts `directory` to `ScriptFile*` -- only works with File objects, NOT constants
- Checks `root->isDirectory()` before proceeding
- Uses `HiseJavascriptEngine::TimeoutExtender` to prevent script timeout during long directory scans
- Calls `root->f.findChildFiles(File::findFilesAndDirectories | File::ignoreHiddenFiles, recursive, wildcard)`
- Filters out `.DS_Store` files explicitly
- Returns `Array<var>` of `ScriptFile` objects
- Returns empty array if argument is not a ScriptFile directory

---

## RSA Encryption Methods

### encryptWithRSA

```cpp
String encryptWithRSA(const String& dataToEncrypt, const String& privateKey)
{
    juce::RSAKey key(privateKey);
    MemoryOutputStream text;
    text << dataToEncrypt;
    BigInteger val;
    val.loadFromMemoryBlock(text.getMemoryBlock());
    key.applyToValue(val);
    return val.toString(16);  // hex string output
}
```

### decryptWithRSA

```cpp
String decryptWithRSA(const String& dataToDecrypt, const String& publicKey)
{
    BigInteger val;
    val.parseString(dataToDecrypt, 16);  // parse hex string
    RSAKey key(publicKey);
    if(key.isValid())
    {
        key.applyToValue(val);
        auto mb = val.toMemoryBlock();
        if (CharPointer_UTF8::isValidString(static_cast<const char*>(mb.getData()), (int)mb.getSize()))
            return mb.toString();
    }
    return {};
}
```

**Notes:**
- Uses JUCE's `RSAKey` / `BigInteger` for raw RSA operations
- Input/output is hex-encoded string (base 16)
- Decrypt validates UTF-8 before returning -- returns empty string if decrypted data is not valid UTF-8
- Decrypt also returns empty if key is invalid
- These are raw RSA operations, not hybrid encryption -- data size is limited by key size

---

## loadExampleAssets

```cpp
void loadExampleAssets()
{
#if USE_BACKEND
    auto am = dynamic_cast<BackendProcessor*>(getMainController())->getAssetManager();
    am->initialise();
#endif
}
```

- **Backend-only** (`#if USE_BACKEND`) -- does nothing in compiled plugins
- Accesses `BackendProcessor::getAssetManager()` which lazily creates an `ExampleAssetManager`
- `ExampleAssetManager` extends `ProjectHandler` and provides dummy audio files, MIDI files, and filmstrips for use in code snippets and examples
- The `initialise()` method populates the asset manager with these resources

---

## getSystemId

```cpp
String getSystemId()
{
    return OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs()[0];
}
```

- Returns the first machine ID from JUCE's machine identification system
- Used for license key validation and computer identification
- Returns a string unique to the machine (hardware-based)

---

## descriptionOfSizeInBytes

```cpp
String descriptionOfSizeInBytes(int64 bytes)
{
    return File::descriptionOfSizeInBytes(bytes);
}
```

- Direct delegation to `juce::File::descriptionOfSizeInBytes`
- Returns human-readable size strings like "1.5 MB", "200 bytes", etc.

---

## getBytesFreeOnVolume

```cpp
int64 getBytesFreeOnVolume(var folder)
```

- Accepts either a SpecialLocations constant (int) or a ScriptFile object
- Delegates to `juce::File::getBytesFreeOnVolume()`
- Returns free bytes on the volume containing the specified folder

---

## findFileSystemRoots

```cpp
var findFileSystemRoots()
{
    Array<File> roots;
    File::findFileSystemRoots(roots);
    Array<var> entries;
    for(auto r: roots)
        entries.add(var(new ScriptFile(getScriptProcessor(), r)));
    return var(entries);
}
```

- Returns array of ScriptFile objects for all filesystem root drives
- On Windows: typically `C:\`, `D:\`, etc.
- On macOS: typically just `/`

---

## Registration Context

FileSystem is registered in ALL script processor types:

1. `JavascriptMidiProcessor` (line 310) -- main interface processor
2. `JavascriptPolyphonicEffect` (line 603) -- polyphonic effect scripts
3. `JavascriptMasterEffect` (line 977) -- master effect scripts
4. `JavascriptTimeVariantModulator` (line ~1873) -- time-variant modulator scripts
5. `JavascriptEnvelopeModulator` (line ~2012) -- envelope modulator scripts

This means FileSystem is universally available in all HiseScript contexts.

---

## Preprocessor Guards Summary

| Guard | Scope | Impact |
|-------|-------|--------|
| `USE_BACKEND` | `getFileStatic` (AppData, UserPresets, AudioFiles cases) | Different directory resolution for backend vs frontend |
| `USE_BACKEND` | `loadExampleAssets` | Entire method body is backend-only |
| `USE_RELATIVE_PATH_FOR_AUDIO_FILES` | `getFileStatic` AudioFiles case in frontend | Controls whether audio files can be loaded from relative paths in compiled plugins; defaults to 1 |

---

## Threading and Lifecycle

- **No onInit restriction** -- FileSystem methods can be called from any callback
- **browse methods are async** -- they use `MessageManager::callAsync()` and deliver results via callback on the message thread
- **browse has static re-entry guard** -- `static bool fileChooserIsOpen` prevents opening multiple dialogs
- **findFiles uses TimeoutExtender** -- prevents script timeout during long directory scans, which could be slow on network drives or large directories
- **getFileFromVar is static** -- can be used by any code with a MainController pointer (used by other API classes like NeuralNetwork)

---

## Key Architectural Patterns

### Dual-mode parameter acceptance
Many methods accept their folder parameter as either:
- An `int` (SpecialLocations constant) -- resolved via `getFileStatic`
- A `ScriptFile*` object -- the underlying `juce::File` is extracted directly

This pattern appears in: `browse`, `browseForDirectory`, `getBytesFreeOnVolume`, and via `getFileFromVar` in `browseForMultipleDirectories`, `browseForMultipleFiles`.

### Factory role
FileSystem is a factory for File objects. Every method that returns a file creates `new ScriptingObjects::ScriptFile(p, juce::File)`. The methods that produce File objects:
- `getFolder(locationType)` -- returns File for a SpecialLocations constant
- `fromAbsolutePath(path)` -- creates File from absolute path string
- `fromReferenceString(ref, type)` -- creates File from HISE reference string
- `findFiles(dir, wildcard, recursive)` -- returns array of Files
- `findFileSystemRoots()` -- returns array of root File objects
- All browse methods -- deliver File object(s) via callback

### FileHandlerBase integration
The class bridges between the scripting API constants and HISE's internal `FileHandlerBase::SubDirectories` enum. The `getSubdirectory()` private method maps scripting constants to internal types, but only for the three pool-compatible directories (AudioFiles, Samples, UserPresets).

---

## FullInstrumentExpansion Impact

When `FullInstrumentExpansion::isEnabled(mc)` returns true, the `Samples` location resolves to the current expansion's samples directory instead of the main project's. This affects `getFolder(FileSystem.Samples)` and any code using `getFileStatic` with the Samples constant.

`FullInstrumentExpansion` is a special expansion type where each expansion contains a complete instrument (preset + samples), rather than just supplementary sample content. It extends `ScriptEncryptedExpansion`.
