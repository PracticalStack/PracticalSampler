# File -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- File entry (createdBy: Download, Expansion, File, FileSystem, Unlocker; seeAlso: FileSystem, Buffer)
- `enrichment/base/File.json` -- 37 API methods
- `enrichment/phase1/FileSystem/Readme.md` -- prerequisite: FileSystem architecture, SpecialLocations resolution, browse dialog patterns
- `enrichment/resources/explorations/FileSystem.md` -- FileSystem C++ exploration (infrastructure already documented there)
- No base class exploration applies (ConstScriptingObject is the direct base, no ScriptComponent chain)

## Source Locations

- **Header:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 324-486
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 190-1127
- **Helper (ValueTreeConverters):** `HISE/hi_scripting/scripting/api/ScriptingBaseObjects.h` lines 667-715
- **Helper (CompressionHelpers::loadFile):** `HISE/hi_lac/hlac/CompressionHelpers.cpp` lines 202-248
- **Helper (FileHandlerBase):** `HISE/hi_core/hi_core/PresetHandler.h` lines 156-235, `PresetHandler.cpp` lines 2568-2724
- **Helper (HiseMidiSequence::TimeSignature):** `HISE/hi_core/hi_dsp/modules/MidiPlayer.h` lines 40-105, `MidiPlayer.cpp` lines 100-170

---

## Class Declaration

```cpp
class ScriptFile : public ConstScriptingObject
```

Nested inside `ScriptingObjects` namespace class. Exposed to HiseScript as `"File"` (via `getObjectName()`).

**Inheritance chain:**
- `ConstScriptingObject` -- provides `addConstant()`, `ADD_API_METHOD_N` registration, `reportScriptError()`, `getScriptProcessor()`
- Through `ConstScriptingObject`: `ScriptingObject` (error reporting, MainController access) and `ApiClass` (method/constant registration)

**Key members:**
- `File f` -- public JUCE `File` member. This is the underlying OS file handle. Most methods are thin wrappers around `f`.
- `struct Wrapper` -- contains `API_METHOD_WRAPPER_N` macro registrations (see Wrapper section below)
- `JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptFile)` -- enables weak referencing, used by extractZipFile's SafeFunctionCall lambda

**Static utility:**
```cpp
static String getFileNameFromFile(var fileOrString);
```
Accepts either a String or a ScriptFile object and returns the full path name. Used internally by other classes that accept file-or-string arguments.

---

## Constructor -- Constants and Method Registration

```cpp
ScriptingObjects::ScriptFile::ScriptFile(ProcessorWithScriptingContent* p, const File& f_) :
    ConstScriptingObject(p, 4),  // 4 constants
    f(f_)
```

### Constants (addConstant calls)

| Script Name | C++ Enum Value | Int Value | Description |
|-------------|---------------|-----------|-------------|
| `FullPath` | `Format::FullPath` | 0 | Full absolute path |
| `NoExtension` | `Format::NoExtension` | 1 | Filename without extension |
| `Extension` | `Format::OnlyExtension` | 2 | File extension only (note: C++ enum is `OnlyExtension`, script constant is `Extension`) |
| `Filename` | `Format::Filename` | 3 | Filename with extension |

**Note the naming discrepancy:** The C++ enum member `OnlyExtension` is registered with the script constant name `"Extension"`:
```cpp
addConstant("Extension", (int)OnlyExtension);
```

### Format Enum Definition (header)

```cpp
enum Format
{
    FullPath,       // 0
    NoExtension,    // 1
    OnlyExtension,  // 2
    Filename        // 3
};
```

These constants are used exclusively by `toString(int formatType)`.

### Method Registration

All methods use plain `ADD_API_METHOD_N` -- no `ADD_TYPED_API_METHOD_N` registrations exist for this class.

Full registration list (37 methods):
```
ADD_API_METHOD_0: getParentDirectory, getSize, getHash, isFile, getBytesFreeOnVolume,
    isDirectory, deleteFileOrDirectory, hasWriteAccess, loadAsString, loadAsObject,
    loadAsAudioFile, loadMidiMetadata, loadAudioMetadata, loadAsBase64String,
    show, getNonExistentSibling, getNumZippedItems, loadFromXmlFile, getRedirectedFolder

ADD_API_METHOD_1: getChildFile, createDirectory, toString, setExecutePermission,
    startAsProcess, writeObject, writeString, loadEncryptedObject, rename,
    move, copy, copyDirectory, isSameFileAs, toReferenceString, getRelativePathFrom,
    loadAsMidiFile

ADD_API_METHOD_2: writeEncryptedObject, isChildOf, setReadOnly, writeAsXmlFile,
    writeMidiFile

ADD_API_METHOD_3: writeAudioFile, extractZipFile
```

### Wrapper Struct

All wrappers are plain `API_METHOD_WRAPPER_N` or `API_VOID_METHOD_WRAPPER_N`:
- Void methods (3): `setReadOnly`, `extractZipFile`, `show`
- Non-void methods (34): all others return a value

---

## Factory Methods / obtainedVia

ScriptFile objects are never created directly by script. They are produced by:

1. **FileSystem.getFolder(location)** -- primary factory, resolves SpecialLocations to File
2. **FileSystem.fromAbsolutePath(path)** -- creates from absolute path string
3. **FileSystem.fromReferenceString(ref, type)** -- creates from pool reference
4. **FileSystem.browse() / browseForDirectory() / browseForMultipleFiles()** -- async callbacks deliver File objects
5. **FileSystem.findFiles(directory, wildcard, recursive)** -- returns array of File objects
6. **File.getChildFile(name)** -- File objects create child File objects
7. **File.getParentDirectory()** -- navigates up the tree
8. **File.createDirectory(name)** -- creates directory and returns File handle
9. **File.getNonExistentSibling()** -- returns a unique sibling
10. **File.getRedirectedFolder()** -- follows HISE link files
11. **Download.getDownloadedTarget()** -- returns the target File from a download operation
12. **Expansion methods** -- various expansion operations return Files

---

## Method Implementation Patterns

### Thin JUCE Wrappers

Most methods are direct 1:1 delegations to `juce::File`:

```
getSize() -> f.getSize()
getBytesFreeOnVolume() -> f.getBytesFreeOnVolume()
setExecutePermission(b) -> f.setExecutePermission(b)
startAsProcess(params) -> f.startAsProcess(params)
hasWriteAccess() -> f.hasWriteAccess()
isFile() -> f.existsAsFile()  // note: maps to existsAsFile(), not isFile()
isDirectory() -> f.isDirectory()
setReadOnly(b, recursive) -> f.setReadOnly(b, recursive)
show() -> MessageManager::callAsync([f_]{ f_.revealToUser(); })  // async dispatch
```

### Methods That Return New ScriptFile Objects

Several methods create and return new ScriptFile instances:
- `getChildFile(name)` -> `new ScriptFile(getScriptProcessor(), f.getChildFile(childFileName))`
- `getParentDirectory()` -> `new ScriptFile(getScriptProcessor(), f.getParentDirectory())`
- `createDirectory(name)` -> creates directory if not exists, returns `new ScriptFile(getScriptProcessor(), f.getChildFile(directoryName))`
- `getNonExistentSibling()` -> `new ScriptFile(getScriptProcessor(), f.getNonexistentSibling(false))`
- `getRedirectedFolder()` -> returns `this` if no redirect, or new ScriptFile wrapping the redirect target

### Methods That Accept ScriptFile Parameters (dynamic_cast pattern)

Several methods accept `var` parameters that must be ScriptFile objects. They use `dynamic_cast<ScriptFile*>(varParam.getObject())`:

- `move(target)` -- casts target, calls `f.moveFileTo(sf->f)`, reports error if not a file
- `copy(target)` -- casts target, calls `f.copyFileTo(sf->f)`, reports error if not a file
- `copyDirectory(target)` -- casts target, additionally checks `sf->f.isDirectory()`, calls `f.copyDirectoryTo(sf->f)`
- `isChildOf(otherFile, checkSubdirectories)` -- if checkSubdirectories: `f.isAChildOf(sf->f)`, else: `f.getParentDirectory() == sf->f`
- `isSameFileAs(otherFile)` -- compares `sf->f == f`
- `getRelativePathFrom(otherFile)` -- requires otherFile to be a directory, returns path with backslashes replaced by forward slashes

---

## Detailed Infrastructure Analysis

### toString(formatType) -- Format Enum Consumption

```cpp
String ScriptingObjects::ScriptFile::toString(int formatType) const
{
    switch (formatType)
    {
    case Format::FullPath:      return f.getFullPathName();
    case Format::NoExtension:   return f.getFileNameWithoutExtension();
    case Format::OnlyExtension: return f.getFileExtension();
    case Format::Filename:      return f.getFileName();
    default:
        reportScriptError("Illegal formatType argument " + String(formatType));
    }
    return {};
}
```

Invalid format types trigger a script error. The switch is exhaustive for the 4 enum values.

### toReferenceString(folderType) -- Pool Reference System

```cpp
String ScriptingObjects::ScriptFile::toReferenceString(String folderType)
{
    FileHandlerBase::SubDirectories dirToUse = FileHandlerBase::SubDirectories::numSubDirectories;

    if (!folderType.endsWithChar('/'))
        folderType << '/';

    for (int i = 0; i < FileHandlerBase::SubDirectories::numSubDirectories; i++)
    {
        if (FileHandlerBase::getIdentifier((FileHandlerBase::SubDirectories)i) == folderType)
        {
            dirToUse = (FileHandlerBase::SubDirectories)i;
            break;
        }
    }

    if (dirToUse != FileHandlerBase::numSubDirectories)
    {
        PoolReference ref(getScriptProcessor()->getMainController_(), f.getFullPathName(), dirToUse);
        return ref.getReferenceString();
    }

    reportScriptError("Illegal folder type");
    RETURN_IF_NO_THROW(var());
}
```

**Key behavior:**
- The `folderType` parameter is a string that must match one of `FileHandlerBase::getIdentifier()` values
- Automatically appends `/` if missing
- Valid folder type strings (from `FileHandlerBase::getIdentifier`):

| SubDirectory Enum | Identifier String |
|-------------------|-------------------|
| Scripts | `"Scripts/"` |
| AdditionalSourceCode | `"AdditionalSourceCode/"` |
| Binaries | `"Binaries/"` |
| Presets | `"Presets/"` |
| XMLPresetBackups | `"XmlPresetBackups/"` |
| Samples | `"Samples/"` |
| Images | `"Images/"` |
| AudioFiles | `"AudioFiles/"` |
| UserPresets | `"UserPresets/"` |
| SampleMaps | `"SampleMaps/"` |
| MidiFiles | `"MidiFiles/"` |
| Documentation | `"Documentation/"` |
| DspNetworks | `"DspNetworks"` |

**Note:** DspNetworks does NOT have a trailing slash in the identifier (the only exception). All others have trailing slashes. The method auto-appends `/` if missing, which means `"DspNetworks"` becomes `"DspNetworks/"` and will NOT match the identifier `"DspNetworks"`. This is likely a bug or an intentionally unsupported directory type for reference strings.

- Creates a `PoolReference` from the file's full path and the matched SubDirectories type
- Returns the reference string from the pool reference (format: `{PROJECT_FOLDER}relativePath`)

### getRedirectedFolder() -- HISE Link File Resolution

```cpp
juce::var ScriptingObjects::ScriptFile::getRedirectedFolder()
{
    if (f.existsAsFile())
        reportScriptError("getRedirectedFolder() must be used with a directory");

    if (!f.isDirectory())
        return var(this);

    auto target = FileHandlerBase::getFolderOrRedirect(f);

    if (target == f)
        return var(this);
    else
        return var(new ScriptFile(getScriptProcessor(), target));
}
```

**Redirect mechanism (from PresetHandler.cpp):**
```cpp
File FileHandlerBase::getLinkFile(const File &subDirectory)
{
#if JUCE_MAC
    return subDirectory.getChildFile("LinkOSX");
#elif JUCE_LINUX
    return subDirectory.getChildFile("LinkLinux");
#else
    return subDirectory.getChildFile("LinkWindows");
#endif
}

File FileHandlerBase::getFolderOrRedirect(const File& folder)
{
    auto lf = getLinkFile(folder);
    if(lf.existsAsFile())
    {
        auto rd = File(lf.loadFileAsString());
        if(rd.isDirectory())
            return rd;
    }
    return folder;
}
```

Link files contain the absolute path to the redirect target as plain text. Platform-specific filenames: `LinkWindows`, `LinkOSX`, `LinkLinux`.

### deleteFileOrDirectory() -- Recursive Deletion

```cpp
bool ScriptingObjects::ScriptFile::deleteFileOrDirectory()
{
    if (!f.isDirectory() && !f.existsAsFile())
        return false;
    return f.deleteRecursively(false);
}
```

Uses `deleteRecursively(false)` -- the `false` parameter means "don't follow symlinks". Returns false if the file/directory doesn't exist.

### rename(newName) -- Sibling Rename with Extension Preservation

```cpp
bool ScriptingObjects::ScriptFile::rename(String newName)
{
    auto newFile = f.getSiblingFile(newName).withFileExtension(f.getFileExtension());
    return f.moveFileTo(newFile);
}
```

**Important behavior:** The original file extension is preserved regardless of what `newName` contains. If `newName` has a different extension, it will be replaced with the original extension.

### getHash() -- SHA256 File Hash

```cpp
String ScriptingObjects::ScriptFile::getHash()
{
    return SHA256(f).toHexString();
}
```

Uses JUCE's `SHA256` class which accepts a `File` directly. Returns a hex string representation of the SHA-256 hash.

### createDirectory(directoryName) -- Directory Creation

```cpp
var ScriptingObjects::ScriptFile::createDirectory(String directoryName)
{
    if (!f.getChildFile(directoryName).isDirectory())
        f.getChildFile(directoryName).createDirectory();
    return new ScriptFile(getScriptProcessor(), f.getChildFile(directoryName));
}
```

Only creates if directory doesn't already exist. Always returns a ScriptFile handle to the child path regardless of whether creation succeeded.

---

## Serialization Methods

### writeString / loadAsString

```cpp
bool ScriptingObjects::ScriptFile::writeString(String text)
{
#if JUCE_LINUX
    return f.replaceWithText(text, false, false, "\n");
#else
    return f.replaceWithText(text);
#endif
}
```

Linux gets explicit `"\n"` line endings. Other platforms use JUCE default (platform-native).

```cpp
String ScriptingObjects::ScriptFile::loadAsString() const
{
    return f.loadFileAsString();
}
```

### writeObject / loadAsObject (JSON)

```cpp
bool ScriptingObjects::ScriptFile::writeObject(var jsonData)
{
    auto text = JSON::toString(jsonData);
    return writeString(text);
}
```

Delegates to `writeString` after JSON serialization.

```cpp
var ScriptingObjects::ScriptFile::loadAsObject() const
{
    var v;
    auto r = JSON::parse(loadAsString(), v);
    if (r.wasOk())
        return v;
    reportScriptError(r.getErrorMessage());
    RETURN_IF_NO_THROW(var());
}
```

Reports script error on parse failure with the JSON error message.

### writeEncryptedObject / loadEncryptedObject (BlowFish)

```cpp
bool ScriptingObjects::ScriptFile::writeEncryptedObject(var jsonData, String key)
{
    auto data = key.getCharPointer().getAddress();
    auto size = jlimit(0, 72, key.length());
    BlowFish bf(data, size);
    auto text = JSON::toString(jsonData, true);
    MemoryOutputStream mos;
    mos.writeString(text);
    mos.flush();
    auto out = mos.getMemoryBlock();
    bf.encrypt(out);
    return f.replaceWithText(out.toBase64Encoding());
}
```

**Encryption details:**
- Uses JUCE's `BlowFish` cipher (symmetric, not RSA)
- Key length clamped to 0-72 bytes via `jlimit`
- JSON is serialized with `compact=true` (the `true` arg to `JSON::toString`)
- Encrypted data is stored as Base64 text
- **NOT the same as FileSystem.encryptWithRSA** -- that uses RSA; this uses BlowFish symmetric encryption

```cpp
var ScriptingObjects::ScriptFile::loadEncryptedObject(String key)
{
    auto data = key.getCharPointer().getAddress();
    auto size = jlimit(0, 72, key.length());
    BlowFish bf(data, size);
    MemoryBlock in;
    in.fromBase64Encoding(f.loadFileAsString());
    bf.decrypt(in);
    var v;
    auto r = JSON::parse(in.toString(), v);
    return v;
}
```

**Note:** loadEncryptedObject does NOT report script errors on parse failure (unlike loadAsObject). It silently returns the var which may be undefined if parsing fails.

### loadAsBase64String

```cpp
String ScriptingObjects::ScriptFile::loadAsBase64String() const
{
    MemoryBlock mb;
    f.loadFileAsData(mb);
    return mb.toBase64Encoding();
}
```

**Doxygen description inaccuracy:** The Doxygen comment says "compresses it with zstd" but the implementation does NO compression. It simply reads the binary file content and Base64-encodes it.

---

## XML Conversion Methods

### writeAsXmlFile(jsonData, tagName)

```cpp
bool ScriptingObjects::ScriptFile::writeAsXmlFile(var jsonDataToBeXmled, String tagName)
{
    ScopedPointer<XmlElement> xml = new XmlElement(tagName);
    auto v = ValueTreeConverters::convertDynamicObjectToValueTree(jsonDataToBeXmled, Identifier(tagName));
    auto s = v.createXml()->createDocument("");
    return writeString(s);
}
```

Uses `ValueTreeConverters::convertDynamicObjectToValueTree` to convert JSON to a ValueTree, then serializes the ValueTree as XML. The `tagName` parameter becomes the root XML element name. The old flat-attribute approach is commented out (`#if 0`).

### loadFromXmlFile()

```cpp
juce::var ScriptingObjects::ScriptFile::loadFromXmlFile()
{
    auto s = loadAsString();
    if (auto xml = XmlDocument::parse(s))
    {
        auto v = ValueTree::fromXml(*xml);
        return ValueTreeConverters::convertValueTreeToDynamicObject(v);
    }
    return var();
}
```

Parses XML text, converts to ValueTree, then converts to dynamic object. Returns undefined if parsing fails (no error reporting).

---

## Audio File Methods

### writeAudioFile(audioData, sampleRate, bitDepth)

Complex method supporting multiple input formats:

**Accepted `audioData` formats:**
1. **Single Buffer** (`audioData.isBuffer()`) -- mono, no buffering needed
2. **Array of Buffers** (`audioData.isArray()` where `audioData[0].isBuffer()`) -- multi-channel, no buffering needed
3. **Array of Arrays** (`audioData.isArray()` where `audioData[0].isArray()`) -- multi-channel with buffering
4. **Plain number Array** (`audioData.isArray()` where `audioData[0]` is number) -- mono with buffering

**Key implementation details:**
- Uses `AudioFormatManager::registerBasicFormats()` -- supports WAV, AIFF, FLAC, OGG (not HLAC)
- Output format is determined by file extension via `afm.findFormatForFileExtension()`
- Deletes existing file before writing
- Validates channel sizes match across multi-channel data
- Uses `FloatSanitizers::sanitizeFloatNumber()` for buffered paths
- Quality parameter of 9 passed to `createWriterFor`
- Reports script errors for: directory target, size mismatch, incompatible data, unknown format

### loadAsAudioFile()

```cpp
juce::var ScriptingObjects::ScriptFile::loadAsAudioFile() const
{
    double unused = 0;
    auto buffer = hlac::CompressionHelpers::loadFile(f, unused);
    // ...
}
```

**Return value depends on channel count:**
- **Mono:** Returns a single `Buffer` (VariantBuffer)
- **Multi-channel:** Returns an `Array` of `Buffer` objects, one per channel

Uses `hlac::CompressionHelpers::loadFile` which supports all JUCE basic formats plus HLAC. Reports "No valid audio file" on failure.

### loadAudioMetadata()

Returns a JSON object with properties:
- `SampleRate` (double) -- file sample rate
- `NumChannels` (int) -- channel count
- `NumSamples` (int64) -- total sample count
- `BitDepth` (int) -- bits per sample
- `Format` (string) -- format name from reader
- `File` (string) -- full path name
- `Metadata` (object) -- all format-specific metadata key-value pairs from the reader

Uses `AudioFormatManager::registerBasicFormats()` and `createReaderFor()`. Returns empty var silently if file doesn't exist or can't be read.

---

## MIDI File Methods

### writeMidiFile(eventList, metadataObject)

```cpp
bool ScriptingObjects::ScriptFile::writeMidiFile(var eventList, var metadataObject)
```

**Parameters:**
- `eventList` -- Array of MessageHolder objects. Each is cast via `dynamic_cast<ScriptingMessageHolder*>` to extract HiseEvents.
- `metadataObject` -- JSON object matching TimeSignature structure (optional properties)

**TimeSignature JSON properties (from TimeSigIds namespace):**

| Property | Default | Description |
|----------|---------|-------------|
| `NumBars` | 0.0 (auto-calculated) | Number of bars |
| `Nominator` | 4.0 | Time signature numerator |
| `Denominator` | 4.0 | Time signature denominator |
| `LoopStart` | 0.0 | Normalised loop start |
| `LoopEnd` | 1.0 | Normalised loop end |
| `Tempo` | 120.0 | BPM |

If `NumBars` is 0, it's auto-calculated from the last event timestamp:
```cpp
if(t.numBars == 0)
{
    t.numBars = hmath::ceil((double)events.getLast().getTimeStamp() / (double)HiseMidiSequence::TicksPerQuarter);
}
```

Uses `HiseMidiSequence::TicksPerQuarter = 960` as internal resolution.

**Process:** Creates HiseMidiSequence, sets time signature, writes events via `MidiPlayer::EditAction::writeArrayToSequence`, writes to temp file, then moves temp file to target.

### loadAsMidiFile(trackIndex)

```cpp
juce::var ScriptingObjects::ScriptFile::loadAsMidiFile(int trackIndex)
```

Only processes `.mid` files (checks extension). Returns a JSON object with:
- `TimeSignature` -- JSON representation of the MIDI file's time signature
- `Events` -- Array of MessageHolder objects

The `trackIndex` parameter is zero-based. Uses `HiseMidiSequence::setCurrentTrackIndex(trackIndex)` then `getEventList(44100.0, 120.0)`.

### loadMidiMetadata()

Loads just the time signature without loading all events. Returns the TimeSignature JSON object directly. Returns undefined if file doesn't exist or can't be parsed.

---

## ZIP Archive Methods

### extractZipFile(targetDirectory, overwriteFiles, callback)

The most complex method in the class. Runs on the **Sample Loading Thread** via `killVoicesAndCall`.

```cpp
void ScriptingObjects::ScriptFile::extractZipFile(var targetDirectory, bool overwriteFiles, var callback)
```

**Target directory parameter:** Accepts either a String (absolute path) or a ScriptFile object.

**Callback protocol:**
The callback receives a JSON object with these properties:

| Property | Type | Description |
|----------|------|-------------|
| `Status` | int | 0 = starting, 1 = extracting, 2 = complete |
| `Progress` | double | 0.0 to 1.0 extraction progress |
| `TotalBytesWritten` | int64 | Running total of extracted bytes |
| `Cancel` | bool | Set to `true` in callback to abort extraction |
| `Target` | string | Target directory path |
| `CurrentFile` | string | Currently extracting filename |
| `Error` | string | Error message if extraction fails |

**Progress reporting strategy:**
- For archives with < 500 entries: callback is called for each file
- For archives with >= 500 entries: callback only called for large entries (> 200MB)
- Large entries (> 200MB) get a `PartUpdater` timer that fires every 200ms with sub-entry progress via `entryProgress`
- Updates `SampleManager::getPreloadProgress()` for global progress tracking

**Cancellation:**
- Checks `Thread::getCurrentThread()->threadShouldExit()` for thread-level abort
- Checks `safeThis == nullptr` for object lifetime
- Checks `data->getProperty("Cancel")` for user cancellation via callback

**Thread safety:**
- Uses `ReferenceCountedObjectPtr<ScriptFile> safeThis(this)` to prevent dangling
- Uses `WeakCallbackHolder` for safe callback invocation
- Dispatched via `KillStateHandler::killVoicesAndCall` on `TargetThread::SampleLoadingThread`

### getNumZippedItems()

```cpp
int ScriptingObjects::ScriptFile::getNumZippedItems()
{
    juce::ZipFile zipFile(f);
    return zipFile.getNumEntries();
}
```

Simple wrapper. Creates a new ZipFile instance each call (not cached).

---

## File Comparison and Path Methods

### isChildOf(otherFile, checkSubdirectories)

```cpp
bool ScriptingObjects::ScriptFile::isChildOf(var otherFile, bool checkSubdirectories) const
{
    if (auto sf = dynamic_cast<ScriptFile*>(otherFile.getObject()))
    {
        if (checkSubdirectories)
            return f.isAChildOf(sf->f);
        else
            return f.getParentDirectory() == sf->f;
    }
    return false;
}
```

When `checkSubdirectories=false`, only checks immediate parent. When `true`, checks the entire ancestor chain.

### getRelativePathFrom(otherFile)

```cpp
String ScriptingObjects::ScriptFile::getRelativePathFrom(var otherFile)
{
    if (auto sf = dynamic_cast<ScriptFile*>(otherFile.getObject()))
    {
        if (!sf->f.isDirectory())
            reportScriptError("otherFile is not a directory");
        auto rp = f.getRelativePathFrom(sf->f);
        return rp.replaceCharacter('\\', '/');
    }
    else
    {
        reportScriptError("otherFile is not a file");
    }
    return {};
}
```

**Important:** Normalizes path separators to forward slashes (cross-platform safety).

### show()

```cpp
void ScriptingObjects::ScriptFile::show()
{
    auto f_ = f;
    MessageManager::callAsync([f_]()
    {
        f_.revealToUser();
    });
}
```

Dispatched asynchronously to the message thread. Uses JUCE `File::revealToUser()` which opens Explorer/Finder.

---

## writeString Platform Behavior

```cpp
bool ScriptingObjects::ScriptFile::writeString(String text)
{
#if JUCE_LINUX
    return f.replaceWithText(text, false, false, "\n");
#else
    return f.replaceWithText(text);
#endif
}
```

On Linux, explicit LF line endings are forced. On Windows/macOS, the default JUCE behavior applies (which uses platform-native endings via `File::replaceWithText` defaults).

---

## PartUpdater Helper (extractZipFile support)

```cpp
struct PartUpdater : public Timer
{
    PartUpdater(const std::function<void()>& f_):
        f(f_)
    {
        startTimer(200);
    }

    ~PartUpdater()
    {
        ScopedLock sl(lock);
        stopTimer();
    }

    std::function<void()> f;

    void timerCallback() override
    {
        ScopedLock sl(lock);
        f();
    }

    CriticalSection lock;
    bool abortFlag = false;
};
```

Local struct defined in the .cpp file scope (not nested in ScriptFile). Used only by `extractZipFile` for periodic progress updates of large zip entries. Fires every 200ms. The `abortFlag` member is declared but never read in the code.

---

## Threading and Lifecycle

### Thread Safety
- Most methods execute synchronously on the calling thread (typically scripting thread)
- `show()` dispatches to the message thread via `MessageManager::callAsync`
- `extractZipFile()` dispatches to the Sample Loading Thread via `killVoicesAndCall`
- No inherent thread safety on the `File f` member -- concurrent access from multiple threads would be unsafe, but this is unlikely given HiseScript's single-threaded execution model

### Lifecycle
- Objects are reference-counted via `ConstScriptingObject` base (which inherits `ReferenceCountedObject`)
- `JUCE_DECLARE_WEAK_REFERENCEABLE` enables weak references used in async operations
- No init-time restrictions -- File objects can be created and used at any point in the script lifecycle
- The underlying `juce::File` is immutable after construction -- methods that "rename" or "move" operate on the OS filesystem, the `f` member itself is not updated

### Important: f is Not Updated After Filesystem Operations

The `File f` member is set once in the constructor and never changed. After calling `rename()`, `move()`, or `deleteFileOrDirectory()`, the ScriptFile object still holds a reference to the **old** path. This is significant:
- After `rename("newName")`: `f` still points to the old filename
- After `move(target)`: `f` still points to the original location
- After `deleteFileOrDirectory()`: `f` still points to the deleted path

---

## Preprocessor Guards

No preprocessor guards (`USE_BACKEND`, etc.) affect the ScriptFile class itself. The only platform-specific code is:
- `#if JUCE_LINUX` in `writeString()` for line ending handling
- The `getLinkFile()` helper uses `#if JUCE_MAC`, `#elif JUCE_LINUX`, `#else` for platform-specific link file names

---

## Upstream Dependencies

### FileHandlerBase::SubDirectories

The `toReferenceString()` method depends on `FileHandlerBase::SubDirectories` enum and `getIdentifier()` for folder type string matching. This is the same enum system documented in the FileSystem prerequisite. Full enum:

```cpp
enum SubDirectories
{
    AudioFiles,          // "AudioFiles/"
    Images,              // "Images/"
    SampleMaps,          // "SampleMaps/"
    MidiFiles,           // "MidiFiles/"
    UserPresets,         // "UserPresets/"
    Samples,             // "Samples/"
    Scripts,             // "Scripts/"
    Binaries,            // "Binaries/"
    Presets,             // "Presets/"
    XMLPresetBackups,    // "XmlPresetBackups/"
    AdditionalSourceCode,// "AdditionalSourceCode/"
    Documentation,       // "Documentation/"
    DspNetworks,         // "DspNetworks" (no trailing slash!)
    numSubDirectories
};
```

### ValueTreeConverters

Used by `writeAsXmlFile` and `loadFromXmlFile` for JSON-to-XML round-tripping. Key methods:
- `convertDynamicObjectToValueTree(var, Identifier)` -- JSON object to ValueTree
- `convertValueTreeToDynamicObject(ValueTree)` -- ValueTree back to JSON

Also used by `writeMidiFile` for deserializing the TimeSignature metadata object.

### HiseMidiSequence

Used by `writeMidiFile`, `loadAsMidiFile`, and `loadMidiMetadata`. Key infrastructure:
- `TicksPerQuarter = 960` -- internal MIDI resolution
- `TimeSignature` struct with NumBars, Nominator, Denominator, LoopStart, LoopEnd, Tempo
- `MidiPlayer::EditAction::writeArrayToSequence` -- converts HiseEvent array to MIDI sequence

### hlac::CompressionHelpers

Used by `loadAsAudioFile`. The `loadFile()` method:
- Reads the entire file into memory
- Uses `AudioFormatManager::registerBasicFormats()` (WAV, AIFF, FLAC, OGG + HLAC via HISE)
- Returns an `AudioSampleBuffer`
- Throws `String` exceptions on failure (caught by HiseScript error handling)

### JUCE BlowFish

Used by `writeEncryptedObject` / `loadEncryptedObject`:
- Symmetric cipher (not public-key)
- Key length capped at 72 bytes
- Data stored as Base64 text on disk

---

## Doxygen Inaccuracies Found

1. **loadAsBase64String:** Doxygen says "compresses it with zstd" -- implementation does NO compression, just Base64 encoding of raw binary
