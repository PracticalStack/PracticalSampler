# Expansion -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- Expansion entry (domain: preset-model, role: handle)
- `enrichment/phase1/ExpansionHandler/Readme.md` -- prerequisite class analysis
- `HISE/hi_scripting/scripting/api/ScriptExpansion.h` -- ScriptExpansionReference class declaration (line 440)
- `HISE/hi_scripting/scripting/api/ScriptExpansion.cpp` -- ScriptExpansionReference implementation (line 1542+)
- `HISE/hi_core/hi_core/ExpansionHandler.h` -- core Expansion class (line 74), ExpansionHandler class (line 209)
- `HISE/hi_core/hi_core/ExpansionHandler.cpp` -- core Expansion implementation
- `HISE/hi_core/hi_core/PresetHandler.h` -- FileHandlerBase class (line 155)

---

## Class Declaration

```cpp
// ScriptExpansion.h:440
class ScriptExpansionReference : public ConstScriptingObject
{
public:
    ScriptExpansionReference(ProcessorWithScriptingContent* p, Expansion* e);

    Identifier getObjectName() const override { return "Expansion"; }
    bool objectDeleted() const override { return exp == nullptr; }
    bool objectExists() const override { return exp != nullptr; }

    BlowFish* createBlowfish();

    // ... 16 API methods ...

private:
    friend class ScriptExpansionHandler;
    struct Wrapper;
    WeakReference<Expansion> exp;
};
```

Key points:
- Scripting object name is `"Expansion"` (returned by getObjectName)
- Wraps core `Expansion` via `WeakReference<Expansion> exp` -- the reference can become null if the expansion is unloaded or destroyed
- `objectDeleted()` / `objectExists()` check the weak reference validity
- `createBlowfish()` is an internal helper (not exposed to scripting) -- dynamic_casts to ScriptEncryptedExpansion
- `friend class ScriptExpansionHandler` -- the handler creates these references

## Inheritance Chain

```
ScriptExpansionReference -> ConstScriptingObject -> DynamicObject
                                                 (provides addConstant, ADD_API_METHOD macros)

Core Expansion class:
Expansion -> FileHandlerBase -> ControlledObject
                             (provides pool system, subdirectory management, link files)
```

## Core Expansion Class (hi_core)

```cpp
// ExpansionHandler.h:74
class Expansion: public FileHandlerBase
{
public:
    enum ExpansionType { FileBased, Intermediate, Encrypted, numExpansionType };

    // Key members:
    File root;                          // expansion root folder
    ScopedPointer<Data> data;           // property data (name, version, tags, etc.)
    int numActiveReferences = 0;        // reference counting for "active" state
    AudioFormatManager afm;             // registers basic + HLAC formats
    // Inherited from FileHandlerBase:
    ScopedPointer<PoolCollection> pool; // all resource pools
};
```

### Expansion Type Hierarchy

Three C++ classes correspond to the three expansion types:

| Type Enum | C++ Class | Info File | Description |
|-----------|-----------|-----------|-------------|
| FileBased (0) | `Expansion` (base) | `expansion_info.xml` | Plain folder with unencrypted resources |
| Intermediate (1) | `ScriptEncryptedExpansion` | `info.hxi` | Encoded ValueTree with pool data |
| Encrypted (2) | `ScriptEncryptedExpansion` | `info.hxp` | Credential-encrypted HXI |

`ScriptEncryptedExpansion` (ScriptExpansion.h:346) extends `Expansion`:
```cpp
class ScriptEncryptedExpansion : public Expansion
{
    Result loadValueTree(ValueTree& v);
    virtual ExpansionType getExpansionType() const; // detects from folder
    void extractUserPresetsIfEmpty(ValueTree encryptedTree, bool forceExtraction = false);
    // ... encoding/decoding infrastructure
};
```

There is also `FullInstrumentExpansion` (extends ScriptEncryptedExpansion) for custom C++ shells that load entire instruments. This is a specialized use case not directly exposed to the Expansion scripting API.

### Type Detection

```cpp
// ExpansionHandler.cpp:708
ExpansionType Expansion::getExpansionTypeFromFolder(const File& f)
{
    if (Helpers::getExpansionInfoFile(f, Encrypted).existsAsFile())  return Encrypted;   // info.hxp
    if (Helpers::getExpansionInfoFile(f, Intermediate).existsAsFile()) return Intermediate; // info.hxi
    if (Helpers::getExpansionInfoFile(f, FileBased).existsAsFile())  return FileBased;    // expansion_info.xml
    jassertfalse;
    return numExpansionType;
}
```

Priority: Encrypted > Intermediate > FileBased (checked in that order).

## Constructor -- Method Registration

```cpp
// ScriptExpansion.cpp:1563
ScriptExpansionReference::ScriptExpansionReference(ProcessorWithScriptingContent* p, Expansion* e) :
    ConstScriptingObject(p, 0),  // 0 = no constants
    exp(e)
{
    ADD_API_METHOD_0(getSampleMapList);
    ADD_API_METHOD_0(getImageList);
    ADD_API_METHOD_0(getAudioFileList);
    ADD_API_METHOD_0(getMidiFileList);
    ADD_API_METHOD_0(getDataFileList);
    ADD_API_METHOD_0(getUserPresetList);
    ADD_API_METHOD_0(getProperties);
    ADD_API_METHOD_1(loadDataFile);
    ADD_API_METHOD_2(writeDataFile);
    ADD_API_METHOD_0(getRootFolder);
    ADD_API_METHOD_0(getExpansionType);
    ADD_API_METHOD_1(getWildcardReference);
    ADD_API_METHOD_1(setSampleFolder);
    ADD_API_METHOD_0(getSampleFolder);
    ADD_API_METHOD_0(rebuildUserPresets);
    ADD_API_METHOD_1(setAllowDuplicateSamples);
    ADD_API_METHOD_0(unloadExpansion);
}
```

**No constants** are added (constructor passes 0 to ConstScriptingObject). The expansion type constants (FileBased, Intermediate, Encrypted) live on ExpansionHandler, not on individual Expansion objects.

**No typed API methods** -- all registrations use plain `ADD_API_METHOD_N` and `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N`.

## Wrapper Struct

```cpp
// ScriptExpansion.cpp:1542
struct ScriptExpansionReference::Wrapper
{
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getSampleMapList);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getImageList);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getAudioFileList);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getMidiFileList);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getDataFileList);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getUserPresetList);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getProperties);
    API_METHOD_WRAPPER_1(ScriptExpansionReference, loadDataFile);
    API_METHOD_WRAPPER_2(ScriptExpansionReference, writeDataFile);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getRootFolder);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getExpansionType);
    API_METHOD_WRAPPER_1(ScriptExpansionReference, getWildcardReference);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, getSampleFolder);
    API_METHOD_WRAPPER_1(ScriptExpansionReference, setSampleFolder);
    API_METHOD_WRAPPER_0(ScriptExpansionReference, rebuildUserPresets);
    API_VOID_METHOD_WRAPPER_0(ScriptExpansionReference, unloadExpansion);
    API_VOID_METHOD_WRAPPER_1(ScriptExpansionReference, setAllowDuplicateSamples);
};
```

## Factory / obtainedVia

ScriptExpansionReference is never created directly by script code. It is created by ScriptExpansionHandler methods:

1. `ScriptExpansionHandler::getExpansionList()` -- returns array of ScriptExpansionReference objects (line 1229)
2. `ScriptExpansionHandler::getExpansion(expansionName)` -- returns single reference by name (line 1239)
3. `ScriptExpansionHandler::getExpansionForInstallPackage(packageFile)` -- returns reference for a package (line 1248)
4. `ScriptExpansionHandler::expansionCallback` -- passes new ScriptExpansionReference to expansion callback (line 1424)
5. `ScriptExpansionHandler::setCurrentExpansion()` -- accepts a ScriptExpansionReference argument (line 1298, dynamic_cast check)
6. Install callback object property "Expansion" (line 1527)

## Pool System (FileHandlerBase)

The `pool` member (`ScopedPointer<PoolCollection>`) inherited from FileHandlerBase provides typed resource pools:

```cpp
// From PresetHandler.h:239
ScopedPointer<PoolCollection> pool;
```

The list methods access these pools:
- `getSampleMapList()` -> `pool->getSampleMapPool().getListOfAllReferences(true)`
- `getImageList()` -> `pool->getImagePool().getListOfAllReferences(true)` (calls loadAllFilesFromProjectFolder first)
- `getAudioFileList()` -> `pool->getAudioSampleBufferPool().getListOfAllReferences(true)` (calls loadAllFilesFromProjectFolder first)
- `getMidiFileList()` -> `pool->getMidiFilePool().getListOfAllReferences(true)`
- `getDataFileList()` -> `pool->getAdditionalDataPool().getListOfAllReferences(true)`
- `getUserPresetList()` -> filesystem scan (NOT pool-based): `getSubDirectory(UserPresets).findChildFiles(..., "*.preset")`

Note: getImageList and getAudioFileList call `loadAllFilesFromProjectFolder()` before listing, which forces discovery of all files. getSampleMapList and getMidiFileList do NOT call this -- they rely on pools being pre-populated during initialise().

## SubDirectories for Expansions

```cpp
// ExpansionHandler.cpp:872
Array<FileHandlerBase::SubDirectories> Expansion::getSubDirectoryIds() const
{
    return { AdditionalSourceCode, Images, AudioFiles, SampleMaps, MidiFiles, Samples, UserPresets };
}
```

The `FileHandlerBase::SubDirectories` enum (PresetHandler.h:160):
- AudioFiles (0), Images (1), SampleMaps (2), MidiFiles (3), UserPresets (4), Samples (5), Scripts (6), AdditionalSourceCode (10), ...

## Wildcard Reference System

```cpp
// ExpansionHandler.cpp:908
String Expansion::getWildcard() const
{
    String s;
    s << "{EXP::" << getProperty(ExpansionIds::Name) << "}";
    return s;
}
```

Format: `{EXP::ExpansionName}relativePath`

The `getWildcardReference()` method simply concatenates: `exp->getWildcard() + relativePath.toString()`

Used to create pool references that resolve to expansion-specific resources.

## Property Object (getProperties)

```cpp
// ExpansionHandler.cpp:867
var Expansion::getPropertyObject() const
{
    return data->toPropertyObject();
}

// ExpansionHandler.cpp:1336
var Expansion::Data::toPropertyObject() const
{
    return ValueTreeConverters::convertValueTreeToDynamicObject(v);
}
```

The Data struct holds CachedValues bound to a ValueTree:

```cpp
// ExpansionHandler.cpp:1308
Expansion::Data::Data(const File& root, ValueTree expansionInfo, MainController* mc) :
    v(expansionInfo),
    name(v, "Name", nullptr, root.getFileNameWithoutExtension()),
    projectName(v, ExpansionIds::ProjectName, nullptr, getProjectName(mc)),
    projectVersion(v, ExpansionIds::ProjectVersion, nullptr, getProjectVersion(mc)),
    tags(v, "Tags", nullptr, ""),
    version(v, "Version", nullptr, "1.0.0")
```

Properties available via getProperties():

| Property | Type | Default | Source |
|----------|------|---------|--------|
| Name | String | folder name | expansion_info.xml |
| ProjectName | String | from project settings | auto-populated |
| ProjectVersion | String | from project settings | auto-populated |
| Tags | String | "" | expansion_info.xml |
| Version | String | "1.0.0" | expansion_info.xml |
| Key | String | (generated) | only if HISE_USE_UNLOCKER_FOR_EXPANSIONS |

Additional properties may exist in the ValueTree depending on the expansion_info.xml content (e.g., Description, Company, URL from ExpansionIds namespace).

## loadDataFile -- Dual Code Path

```cpp
// ScriptExpansion.cpp:1753
var ScriptExpansionReference::loadDataFile(var relativePath)
{
    if (objectExists())
    {
        if (exp->getExpansionType() == Expansion::FileBased)
        {
            // FileBased: read directly from filesystem
            auto fileToLoad = exp->getSubDirectory(FileHandlerBase::AdditionalSourceCode)
                                  .getChildFile(relativePath.toString());
            if(fileToLoad.existsAsFile())
                return JSON::parse(fileToLoad.loadFileAsString());
        }
        else
        {
            // Intermediate/Encrypted: load from pool with wildcard reference
            String rs;
            auto wc = exp->getWildcard();
            auto path = relativePath.toString();
            if (!path.contains(wc))
                rs << wc;
            rs << path;
            PoolReference ref(getScriptProcessor()->getMainController_(), rs, 
                             FileHandlerBase::AdditionalSourceCode);
            if (auto o = exp->pool->getAdditionalDataPool().loadFromReference(ref, 
                         PoolHelpers::LoadAndCacheStrong))
            {
                var obj;
                auto ok = JSON::parse(o->data.getFile(), obj);
                if (ok.wasOk()) return obj;
                reportScriptError("Error at parsing JSON: " + ok.getErrorMessage());
            }
        }
    }
    return {};
}
```

Key behavioral difference: FileBased reads from the filesystem AdditionalSourceCode folder; Intermediate/Encrypted loads from the embedded pool data. Both parse JSON.

## writeDataFile -- Filesystem Only

```cpp
// ScriptExpansion.cpp:1802
bool ScriptExpansionReference::writeDataFile(var relativePath, var dataToWrite)
{
    auto content = JSON::toString(dataToWrite);
    auto targetFile = exp->getSubDirectory(FileHandlerBase::AdditionalSourceCode)
                         .getChildFile(relativePath.toString());
    return targetFile.replaceWithText(content);
}
```

Always writes to the filesystem AdditionalSourceCode directory. No pool involvement. Works for all expansion types since even Intermediate/Encrypted expansions have a root folder.

## rebuildUserPresets -- Encrypted/Intermediate Only

```cpp
// ScriptExpansion.cpp:1827
bool ScriptExpansionReference::rebuildUserPresets()
{
    if (auto sf = dynamic_cast<ScriptEncryptedExpansion*>(exp.get()))
    {
        ValueTree v;
        auto ok = sf->loadValueTree(v);
        if (ok.wasOk())
        {
            sf->extractUserPresetsIfEmpty(v, true); // forceExtraction = true
            return true;
        }
        else
        {
            debugError(dynamic_cast<Processor*>(getScriptProcessor()), 
                      "Error at extracting user presets: ");
            debugError(dynamic_cast<Processor*>(getScriptProcessor()), 
                      ok.getErrorMessage());
        }
    }
    return false;
}
```

Only works when the underlying Expansion is a `ScriptEncryptedExpansion` (Intermediate or Encrypted type). For FileBased expansions, the dynamic_cast fails and returns false silently.

The `forceExtraction = true` parameter means it overrides existing user presets.

## setSampleFolder -- Link File Creation

```cpp
// ScriptExpansion.cpp:1733
bool ScriptExpansionReference::setSampleFolder(var newSampleFolder)
{
    if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(newSampleFolder.getObject()))
    {
        auto newTarget = f->f;
        if (!newTarget.isDirectory())
            reportScriptError(newTarget.getFullPathName() + " is not an existing directory");
        if (newTarget != exp->getSubDirectory(FileHandlerBase::Samples))
        {
            exp->createLinkFile(FileHandlerBase::Samples, newTarget);
            exp->checkSubDirectories();
            return true;
        }
    }
    return false;
}
```

Requires a `ScriptFile` object (not a string path). Creates a link file in the Samples subdirectory pointing to the new location, then refreshes subdirectory state.

## getRootFolder / getSampleFolder -- Return ScriptFile

```cpp
var ScriptExpansionReference::getRootFolder()
{
    if (objectExists())
        return var(new ScriptingObjects::ScriptFile(getScriptProcessor(), exp->getRootFolder()));
    reportScriptError("Expansion was deleted");
    RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getSampleFolder()
{
    File sampleFolder = exp->getSubDirectory(FileHandlerBase::Samples);
    return new ScriptingObjects::ScriptFile(getScriptProcessor(), sampleFolder);
}
```

Both return `ScriptFile` objects (the HISE `File` scripting type). getSampleFolder does NOT check objectExists() (potential null deref if expansion is deleted -- though WeakReference would catch this at access).

## setAllowDuplicateSamples

```cpp
void ScriptExpansionReference::setAllowDuplicateSamples(bool shouldAllowDuplicates)
{
    if (exp != nullptr)
        exp->pool->getSamplePool()->setAllowDuplicateSamples(shouldAllowDuplicates);
}
```

Delegates to the SamplePool. Controls whether multiple expansions can reference the same sample files. Setting false means the pool will reject duplicate sample references.

## unloadExpansion

```cpp
void ScriptExpansionReference::unloadExpansion()
{
    if (exp != nullptr)
        exp->getMainController()->getExpansionHandler().unloadExpansion(exp);
}
```

Delegates to ExpansionHandler::unloadExpansion(). After this call, the expansion won't appear in getExpansionList() until next restart. The WeakReference `exp` becomes null after unloading.

## List Method Return Format Details

All list methods (getSampleMapList, getImageList, etc.) return `Array<var>` containing strings. The string format depends on the pool type:

- **getSampleMapList**: reference string with `.xml` extension stripped
- **getImageList**: reference string as-is
- **getAudioFileList**: reference string as-is
- **getMidiFileList**: reference string as-is
- **getDataFileList**: reference string as-is
- **getUserPresetList**: relative path from UserPresets folder, `.preset` stripped, backslashes replaced with forward slashes

## ExpansionIds Namespace

```cpp
// ExpansionHandler.h:43
namespace ExpansionIds
{
    DECLARE_ID(ExpansionInfo);  DECLARE_ID(FullData);
    DECLARE_ID(Preset);         DECLARE_ID(Scripts);
    DECLARE_ID(Script);         DECLARE_ID(HeaderData);
    DECLARE_ID(Fonts);          DECLARE_ID(Icon);
    DECLARE_ID(HiseVersion);    DECLARE_ID(Credentials);
    DECLARE_ID(PrivateInfo);    DECLARE_ID(Name);
    DECLARE_ID(ProjectName);    DECLARE_ID(ProjectVersion);
    DECLARE_ID(Version);        DECLARE_ID(Tags);
    DECLARE_ID(Key);            DECLARE_ID(Hash);
    DECLARE_ID(PoolData);       DECLARE_ID(Data);
    DECLARE_ID(URL);            DECLARE_ID(UUID);
    DECLARE_ID(Description);    DECLARE_ID(Company);
    DECLARE_ID(CompanyURL);
}
```

These identifiers are used in the ValueTree that backs expansion properties. The `toPropertyObject()` method converts the entire ValueTree to a dynamic object, so any of these properties may appear in the returned object depending on the expansion_info.xml content.

## Preprocessor Guards

No preprocessor guards affect the ScriptExpansionReference class itself. However:

- `HISE_USE_UNLOCKER_FOR_EXPANSIONS` -- affects whether the `Key` property is generated in Expansion::Data constructor
- `HISE_USE_CUSTOM_EXPANSION_TYPE` -- affects ExpansionHandler's factory method for creating Expansion objects
- `USE_BACKEND` -- affects Data::getProjectVersion() and Data::getProjectName() (backend reads from settings, frontend from FrontendHandler)
- `HISE_USE_XML_FOR_HXI` -- affects ScriptEncryptedExpansion encoding (not directly relevant to this wrapper)

## Threading / Lifecycle

- The ScriptExpansionReference is a scripting-thread object (created/accessed from script callbacks)
- The underlying `Expansion` object is owned by ExpansionHandler and can be destroyed asynchronously
- WeakReference pattern protects against dangling access -- most methods check `objectExists()` or `exp != nullptr`
- `getSampleFolder()` does NOT check objectExists() before accessing exp (minor inconsistency)
- No explicit thread safety in the wrapper -- relies on HISE's scripting thread model
- `rebuildUserPresets` calls `loadValueTree` which may involve file I/O or decryption

## Relationship to ExpansionHandler

Per the prerequisite analysis, ExpansionHandler is the factory that manages the collection. Key integration points:

- ExpansionHandler creates ScriptExpansionReference objects and passes them to scripts
- ExpansionHandler.setCurrentExpansion() accepts a ScriptExpansionReference (checked via dynamic_cast)
- The expansion callback receives a ScriptExpansionReference as its argument
- The install callback object has an "Expansion" property containing a ScriptExpansionReference
- unloadExpansion() delegates back to ExpansionHandler
- Expansion type constants (FileBased=0, Intermediate=1, Encrypted=2) are on ExpansionHandler, not Expansion
