# ExpansionHandler -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisite check (row 6: ExpansionHandler -> Expansion)
- `enrichment/resources/survey/class_survey_data.json` -- ExpansionHandler entry (createdBy: Engine, creates: Expansion)
- `enrichment/phase1/Engine/Readme.md` -- prerequisite class context
- `enrichment/base/ExpansionHandler.json` -- 17 methods in base JSON

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptExpansion.h`, line 202

```cpp
class ScriptExpansionHandler : public ConstScriptingObject,
                               public ControlledObject,
                               public ExpansionHandler::Listener
```

### Inheritance

- **ConstScriptingObject** -- standard scripting API base; provides `addConstant()`, `ADD_API_METHOD_N` registration
- **ControlledObject** -- provides `getMainController()` access
- **ExpansionHandler::Listener** -- receives callbacks for expansion lifecycle events

### Object Name

Returns `"ExpansionHandler"` via `getObjectName()`.

### Member Variables

```cpp
WeakCallbackHolder errorFunction;       // 2-arg callback: (message, isCritical)
WeakCallbackHolder expansionCallback;   // 1-arg callback: (expansion)
WeakCallbackHolder installCallback;     // 1-arg callback: (installStateObject)
WeakReference<JavascriptProcessor> jp;
ScopedPointer<InstallState> currentInstaller;
```

## Factory / obtainedVia

Created by `Engine.createExpansionHandler()` (ScriptingApi.cpp:2445):

```cpp
var ScriptingApi::Engine::createExpansionHandler()
{
    return var(new ScriptExpansionHandler(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())));
}
```

Also used internally by `Engine.getExpansionList()` which calls `createExpansionHandler()` then delegates.

## Constructor (ScriptExpansion.cpp:1142)

```cpp
ScriptExpansionHandler::ScriptExpansionHandler(JavascriptProcessor* jp_) :
    ConstScriptingObject(dynamic_cast<ProcessorWithScriptingContent*>(jp_), 3),  // 3 constants
    ControlledObject(dynamic_cast<ControlledObject*>(jp_)->getMainController()),
    jp(jp_),
    expansionCallback(dynamic_cast<ProcessorWithScriptingContent*>(jp_), nullptr, var(), 1),
    errorFunction(dynamic_cast<ProcessorWithScriptingContent*>(jp_), nullptr, var(), 2),
    installCallback(dynamic_cast<ProcessorWithScriptingContent*>(jp_), nullptr, var(), 1)
```

The constructor:
1. Registers as a listener on `getMainController()->getExpansionHandler()`
2. Registers all API methods
3. Adds 3 constants via `addConstant()`

### Constants Registered

```cpp
addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::FileBased), Expansion::FileBased);
addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::Intermediate), Expansion::Intermediate);
addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::Encrypted), Expansion::Encrypted);
```

From `Expansion::Helpers::getExpansionTypeName()` (ExpansionHandler.cpp:1226):
- `"FileBased"` -> 0 (Expansion::FileBased)
- `"Intermediate"` -> 1 (Expansion::Intermediate)
- `"Encrypted"` -> 2 (Expansion::Encrypted)

These map to the `Expansion::ExpansionType` enum (ExpansionHandler.h:78):
```cpp
enum ExpansionType
{
    FileBased,      // 0 -- plain folder with expansion_info.xml
    Intermediate,   // 1 -- .hxi file (intermediate encoded)
    Encrypted,      // 2 -- .hxp file (credential-encrypted)
    numExpansionType
};
```

### Typed API Methods (ADD_TYPED_API_METHOD)

```cpp
ADD_TYPED_API_METHOD_1(setErrorFunction, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_1(setExpansionCallback, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_1(setInstallCallback, VarTypeChecker::Function);
```

All three typed methods require `Function` type.

### Callback Diagnostics

```cpp
ADD_CALLBACK_DIAGNOSTIC(errorFunction, setErrorFunction, 0);
ADD_CALLBACK_DIAGNOSTIC(expansionCallback, setExpansionCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(installCallback, setInstallCallback, 0);
```

### Unregistered Method: getUninitialisedExpansions

The method `getUninitialisedExpansions()` is declared in the header (line 247), implemented (line 1278), and present in the base JSON, but it is NOT registered in the constructor via `ADD_API_METHOD_0` and NOT in the Wrapper struct. This means it may not be callable from script despite being in the Doxygen output.

## Wrapper Struct (ScriptExpansion.cpp:1121)

All methods use standard `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` -- no typed wrappers at the wrapper level.

```
API_VOID_METHOD_WRAPPER_1: setErrorFunction, setErrorMessage, setCredentials,
                           setInstallFullDynamics, setEncryptionKey,
                           setExpansionCallback, setInstallCallback,
                           setAllowedExpansionTypes
API_METHOD_WRAPPER_0:      getExpansionList, getCurrentExpansion, refreshExpansions
API_METHOD_WRAPPER_1:      setCurrentExpansion, getExpansion, encodeWithCredentials,
                           getExpansionForInstallPackage, getMetaDataFromPackage
API_METHOD_WRAPPER_2:      installExpansionFromPackage
```

## Core Infrastructure: ExpansionHandler (hi_core)

**File:** `HISE/hi_core/hi_core/ExpansionHandler.h`, line 209

```cpp
class ExpansionHandler: public hlac::HlacArchiver::Listener,
                        public ControlledObject
```

The core `ExpansionHandler` (not `ScriptExpansionHandler`) lives in `hi_core` and is the actual expansion management engine. `ScriptExpansionHandler` is a thin scripting wrapper that delegates to `getMainController()->getExpansionHandler()`.

### Key State

- `OwnedArray<Expansion> expansionList` -- successfully initialized expansions
- `OwnedArray<Expansion> uninitialisedExpansions` -- failed to initialize (missing key, etc.)
- `OwnedArray<Expansion> unloadedExpansions` -- explicitly unloaded at runtime
- `WeakReference<Expansion> currentExpansion` -- the active expansion (or nullptr)
- `var credentials` -- JSON object for credential-based encryption
- `String keyCode` -- BlowFish encryption key (deprecated path)
- `bool installFullDynamics` -- whether to extract full dynamic range samples
- `double totalProgress` -- installation progress tracking
- `Array<Expansion::ExpansionType> allowedExpansions` -- type filter

### Expansion Folder Resolution

`getExpansionFolder()` (ExpansionHandler.cpp:180):
1. Gets `{ProjectRoot}/Expansions/` from the project handler
2. Checks for platform-specific link files (`LinkWindows`, `LinkLinux`, `LinkOSX`)
3. If a link file exists, redirects to the linked folder
4. Creates the directory if it doesn't exist (unless `DONT_CREATE_EXPANSIONS_FOLDER` is defined)

### Expansion Discovery: createAvailableExpansions()

(ExpansionHandler.cpp:208)
1. Scans the expansion folder for child directories
2. Skips already-known expansions
3. For each valid expansion folder, calls `createExpansionForFile()`
4. Sorts expansions alphabetically by name
5. Sends `ExpansionCreated` notification

### Expansion Creation: createExpansionForFile()

(ExpansionHandler.cpp:286)
1. If `HISE_USE_CUSTOM_EXPANSION_TYPE` is defined, delegates to `createCustomExpansion()`
2. Otherwise uses the `expansionCreateFunction` lambda set by `setExpansionType<T>()`
3. Calls `e->initialise()` on the new expansion
4. If initialisation fails, moves to `uninitialisedExpansions` list with the error

### Setting Current Expansion

`setCurrentExpansion(const String&)` (ExpansionHandler.cpp:340):
- Empty string with existing expansion: clears current, notifies
- Finds expansion by name in `expansionList`
- Calls overload `setCurrentExpansion(Expansion*, NotificationType)`

`setCurrentExpansion(Expansion*)` (ExpansionHandler.cpp:363):
- If first time setting (from null), saves current state via `FullInstrumentExpansion::setNewDefault()`
- Checks HISE version compatibility between expansion and current build
- Sends `ExpansionLoaded` notification

### Listener Interface

```cpp
class Listener {
    virtual void expansionPackCreated(Expansion* newExpansion);
    virtual void expansionPackLoaded(Expansion* currentExpansion);
    virtual void expansionInstalled(Expansion* newExpansion);
    virtual void expansionInstallStarted(const File& targetRoot, const File& packageFile, const File& sampleDirectory);
    virtual void logMessage(const String& message, bool isCritical);
};
```

### Notifier (async event dispatch)

Internal `Notifier` class with two event types:
- `ExpansionLoaded` -- sent when `setCurrentExpansion()` changes the active expansion
- `ExpansionCreated` -- sent when new expansions are discovered or initialised

Uses `AsyncUpdater` for thread-safe notification delivery.

## Expansion Type Hierarchy

From ExpansionHandler.h:

1. **Expansion** (base) -- `FileBased` type, plain folder with `expansion_info.xml`
   - Inherits `FileHandlerBase` -- provides pool management and subdirectory access
   - Subdirectories: AdditionalSourceCode, Images, AudioFiles, SampleMaps, MidiFiles, Samples, UserPresets

2. **ScriptEncryptedExpansion** (ScriptExpansion.h:346) -- `Intermediate` and `Encrypted` types
   - Extends Expansion with ValueTree-based loading, BlowFish encryption
   - `.hxi` files (intermediate) and `.hxp` files (encrypted with credentials)

3. **FullInstrumentExpansion** (ScriptExpansion.h:386) -- special expansion type
   - Includes the full instrument preset (patch), not just resources
   - Used for C++ shell products loading different instruments as expansions
   - Has `DefaultHandler` inner class for restoring default state when no expansion is loaded

## Installation Flow

### installExpansionFromPackage (ScriptExpansion.cpp:1323)

1. Resolves `sampleDirectory`: accepts `FileSystem.Expansions`, `FileSystem.Samples`, or a File object
2. If an install callback is registered, creates `InstallState` object
3. Delegates to `ExpansionHandler::installFromResourceFile()`

### ExpansionHandler::installFromResourceFile (ExpansionHandler.cpp:409)

Runs on the **SampleLoadingThread** via `killVoicesAndCall()`:
1. Creates expansion directory structure
2. Creates Samples subdirectory with optional link file for custom sample location
3. Notifies listeners via `expansionInstallStarted()`
4. Decompresses the `.hr` archive using `HlacArchiver`
5. If credentials are set (and not using Unlocker), encrypts the intermediate file to `.hxp`
6. Otherwise renames `header.dat` to `.hxi`
7. Calls `forceReinitialisation()` to reload expansions
8. Notifies listeners via `expansionInstalled()`

### InstallState Inner Class (ScriptExpansion.h:290)

Tracks installation progress and feeds the install callback:

```cpp
struct InstallState: public Timer, public ExpansionHandler::Listener
```

- Registers as an `ExpansionHandler::Listener`
- Timer fires every 300ms during installation
- Constructs a status object for the callback

#### Install Callback Object Schema

From `InstallState::getObject()` (ScriptExpansion.cpp:1518):

| Property | Type | Description |
|----------|------|-------------|
| Status | int | -1 = not started, 0 = started, 1 = in progress, 2 = complete |
| Progress | double | Sample preload progress (0.0-1.0) |
| TotalProgress | double | Overall installation progress |
| SourceFile | File | The .hr package file |
| TargetFolder | File | The expansion root folder |
| SampleFolder | File | The sample destination folder |
| Expansion | Expansion or undefined | The created expansion (only when Status=2) |

## Credential System

### setCredentials (ScriptExpansion.cpp:1191)

Validates that the argument is a `DynamicObject`, then stores via `ExpansionHandler::setCredentials()`.

### ExpansionHandler Credential Storage

- `var credentials` -- stored as a JSON/DynamicObject
- `getCredentials()` returns the stored var
- Used during install to decide whether to encrypt intermediate files to `.hxp`
- With `HISE_USE_UNLOCKER_FOR_EXPANSIONS`, credentials become per-expansion keys indexed by expansion slug

### encodeWithCredentials (ScriptExpansion.cpp:1307)

Takes a File (`.hxi` file), calls `ScriptEncryptedExpansion::encryptIntermediateFile()`.
Only works with File objects -- reports error otherwise.

## Error Handling

### setErrorFunction (ScriptExpansion.cpp:1207)

Stores a `WeakCallbackHolder` with 1 argument capacity (but actually called with 2 args).
Marked as high priority.

### logMessage (ScriptExpansion.cpp:1437)

Called by the ExpansionHandler::Listener interface and by `setErrorMessage()`:
```cpp
void ScriptExpansionHandler::logMessage(const String& message, bool isCritical)
{
    if (errorFunction)
    {
        var args[2];
        args[0] = message;      // String
        args[1] = isCritical;   // bool
        errorFunction.call(args, 2);
    }
}
```

The error function callback receives 2 arguments: `(message, isCritical)`.

### Expansion Callback

`expansionPackLoaded()` and `expansionPackCreated()` both call `expansionPackCreated()`:
- If expansion is not null: passes `ScriptExpansionReference` as argument
- If expansion is null: passes `undefined`

So the expansion callback receives a single argument: the loaded/created Expansion object (or undefined when deselected).

## Deprecated Method: setEncryptionKey

```cpp
void ScriptExpansionHandler::setEncryptionKey(String newKey)
{
    reportScriptError("This function is deprecated. Use the project settings to setup the project's blowfish key");
}
```

Always throws an error. The key is now set through project settings, not script.

## setAllowedExpansionTypes (ScriptExpansion.cpp:1398)

Takes an array of expansion type constants (the integer values from the registered constants).
Delegates to `ExpansionHandler::setAllowedExpansions()`.
Reports script error if argument is not an array.

### Behavioral Impact of Allowed Types

`ExpansionHandler::checkAllowedExpansions()` (ExpansionHandler.cpp:677) is called during:
- `createExpansionForFile()` -- when first discovering expansions
- `forceReinitialisation()` -- when reinitializing after install
- `rebuildUnitialisedExpansions()` -- when retrying failed expansions

If an expansion's type is not in the allowed list, its `Result` is set to failed and it moves to `uninitialisedExpansions`. This effectively filters which expansion types are visible.

## setCurrentExpansion Overloading (ScriptExpansion.cpp:1293)

Accepts either a String (expansion name) or a ScriptExpansionReference object:
```cpp
bool ScriptExpansionHandler::setCurrentExpansion(var expansionName)
{
    if(expansionName.isString())
        return getMainController()->getExpansionHandler().setCurrentExpansion(expansionName);
    
    if (auto se = dynamic_cast<ScriptExpansionReference*>(expansionName.getObject()))
        return setCurrentExpansion(se->exp->getProperty(ExpansionIds::Name));

    reportScriptError("can't find expansion");
    RETURN_IF_NO_THROW(false);
}
```

Passing an empty string clears the current expansion.

## getExpansionForInstallPackage (ScriptExpansion.cpp:1371)

1. Reads package metadata to get the target folder
2. Checks if an expansion already exists at that location
3. Returns the expansion reference if found, BUT excludes `FileBased` expansions
4. Returns undefined if not found or if the expansion is FileBased

The FileBased exclusion is deliberate -- simulates end-user flow where file-based expansions don't represent installed packages.

## getMetaDataFromPackage (ScriptExpansion.cpp:1360)

Creates an `HlacArchiver` and reads metadata directly from the `.hr` archive file.
Returns the metadata as a var (JSON object with `HxiName` property, etc.).

## Preprocessor Guards

- `USE_BACKEND` -- controls `reportScriptError()` behavior (throws in backend, no-op in frontend)
- `HISE_USE_CUSTOM_EXPANSION_TYPE` -- enables custom C++ expansion type via `createCustomExpansion()` (default: 0)
- `HISE_USE_UNLOCKER_FOR_EXPANSIONS` -- per-expansion key management via Unlocker system (default: 0)
- `DONT_CREATE_EXPANSIONS_FOLDER` -- prevents auto-creation of Expansions folder
- `HISE_USE_XML_FOR_HXI` -- controls HXI data format (ValueTree vs XML, default: 0)
- `HI_ENABLE_EXPANSION_EDITING` -- controls expansion editing UI in backend

## ExpansionIds Namespace

Defined in ExpansionHandler.h:43:

| Identifier | Used For |
|------------|----------|
| ExpansionInfo | Root ValueTree tag |
| FullData | Full expansion data container |
| Preset | Embedded preset data |
| Scripts | Script container |
| Script | Individual script |
| HeaderData | Header section |
| Fonts | Embedded fonts |
| Icon | Expansion icon |
| HiseVersion | HISE build version |
| Credentials | Credential data |
| PrivateInfo | Private metadata |
| Name | Expansion name |
| ProjectName | Parent project name |
| ProjectVersion | Parent project version |
| Version | Expansion version |
| Tags | Expansion tags |
| Key | Encryption key |
| Hash | Data hash |
| PoolData | Pool data container |
| Data | Generic data |
| URL | Expansion URL |
| UUID | Unique identifier |
| Description | Expansion description |
| Company | Company name |
| CompanyURL | Company website |

## Threading Considerations

- `installFromResourceFile()` runs on the SampleLoadingThread via `killVoicesAndCall()`
- InstallState timer runs on the message thread (JUCE Timer)
- `InstallState::expansionInstalled()` uses `SimpleReadWriteLock` to coordinate with the timer
- Expansion callback and error function are `WeakCallbackHolder` with high priority
- The expansion callback is marked with `setThisObject(this)` and `addAsSource()`

## FullInstrumentExpansion Context

From ScriptExpansion.h:386, this is a special expansion type that:
- Contains the full instrument preset (ValueTree), not just resource pools
- Used for C++ shell products where each expansion is a different instrument
- Has a `DefaultHandler` that restores the default preset when switching away
- Checked via `FullInstrumentExpansion::isEnabled(mc)` which affects wildcard resolution behavior
