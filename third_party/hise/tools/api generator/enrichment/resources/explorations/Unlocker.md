# Unlocker -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- Unlocker entry (domain: preset-model, role: service)
- `enrichment/base/Unlocker.json` -- 15 API methods
- No prerequisites required for this class

## Source Files

- **Header:** `HISE/hi_scripting/scripting/api/ScriptExpansion.h` (lines 560-671)
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptExpansion.cpp` (lines 3200-3807)
- **Factory:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` (line 2179-2182)
- **Preprocessor config:** `HISE/hi_core/hi_core.h` (lines 146-178)
- **Copy protection macros:** `HISE/hi_core/copyProtectionMacros.h`

## Class Architecture

### Two-Layer Design: ScriptUnlocker + RefObject

The Unlocker API uses a **two-layer pattern**:

1. **`ScriptUnlocker`** -- The actual license manager. Inherits from `juce::OnlineUnlockStatus`, `UnlockerHandler`, and `ControlledObject`. This is a singleton owned by the processor (BackendProcessor or FrontendProcessor). It implements all the JUCE virtual methods for RSA key validation.

2. **`ScriptUnlocker::RefObject`** -- The scripting API wrapper. Inherits from `ConstScriptingObject`. This is the object returned to HiseScript via `Engine.createLicenseUnlocker()`. It holds a `WeakReference<ScriptUnlocker>` to the actual unlocker.

The scripting API name is `"Unlocker"` (from `getObjectName()`), but the C++ class exposing the API is `ScriptUnlocker::RefObject`.

### Inheritance Chain

```
ScriptUnlocker
  : public juce::OnlineUnlockStatus   -- JUCE RSA license validation
  : public UnlockerHandler             -- Interface { getUnlockerObject() -> OnlineUnlockStatus* }
  : public ControlledObject            -- Access to MainController

ScriptUnlocker::RefObject
  : public ConstScriptingObject        -- Standard HISE scripting API base
```

### UnlockerHandler Interface

```cpp
struct UnlockerHandler
{
    virtual ~UnlockerHandler() {};
    virtual juce::OnlineUnlockStatus* getUnlockerObject() = 0;
};
```

This is a simple interface that allows polymorphic access to the unlocker. Both `ScriptUnlocker` and the non-script `Unlocker` class (in `FrontEndProcessor.h`) implement this.

## Factory / obtainedVia

Created via `Engine.createLicenseUnlocker()`:

```cpp
// ScriptingApi.cpp:2179
juce::var ScriptingApi::Engine::createLicenseUnlocker()
{
    return var(new ScriptUnlocker::RefObject(getScriptProcessor()));
}
```

The `RefObject` constructor obtains the `ScriptUnlocker` singleton from `MainController::getLicenseUnlocker()`:

```cpp
// ScriptExpansion.cpp:3524-3538
ScriptUnlocker::RefObject::RefObject(ProcessorWithScriptingContent* p) :
    ConstScriptingObject(p, 0),   // 0 constants
#if USE_BACKEND || USE_COPY_PROTECTION
    unlocker(dynamic_cast<ScriptUnlocker*>(p->getMainController_()->getLicenseUnlocker())),
#endif
    pcheck(p, nullptr, var(), 1),
    mcheck(p, nullptr, var(), 1)
{
    if (unlocker->getLicenseKeyFile().existsAsFile())
        unlocker->loadKeyFile();

    if(HISE_GET_PREPROCESSOR(getScriptProcessor()->getMainController_(), HISE_USE_UNLOCKER_FOR_EXPANSIONS))
        loadExpansionList();

    unlocker->currentObject = this;
    // ... ADD_API_METHOD registrations
}
```

Key points:
- Constructor **automatically loads the key file** if it exists
- Constructor **automatically loads the expansion list** if `HISE_USE_UNLOCKER_FOR_EXPANSIONS` is enabled
- Sets `unlocker->currentObject = this` to receive callbacks (product check, MuseHub)

## Ownership: Backend vs Frontend

### BackendProcessor (USE_BACKEND)

```cpp
// BackendProcessor.h:537-540,546
juce::OnlineUnlockStatus* getLicenseUnlocker()
{
    return &scriptUnlocker;
}
ScriptUnlocker scriptUnlocker;
```

The `ScriptUnlocker` is always available in the backend, regardless of copy protection settings.

### FrontendProcessor (USE_FRONTEND)

```cpp
// FrontEndProcessor.h:38-214
#if USE_COPY_PROTECTION
    #if USE_SCRIPT_COPY_PROTECTION
        using Unlocker = hise::ScriptUnlocker;   // ScriptUnlocker IS the unlocker
    #else
        class Unlocker : public UnlockerHandler   // Separate non-script Unlocker class
        { ... };
    #endif

    juce::OnlineUnlockStatus* getLicenseUnlocker() override { return unlocker.getUnlockerObject(); };
    Unlocker unlocker;
#endif
```

The `getLicenseUnlocker()` is only available in frontend when `USE_COPY_PROTECTION` is enabled. When `USE_SCRIPT_COPY_PROTECTION` is true, the `ScriptUnlocker` itself is used directly; otherwise a separate `Unlocker` class with built-in UI overlay handles it.

## Constructor -- API Method Registration

The RefObject constructor registers 15 methods, all using `ADD_API_METHOD_N` (no typed variants):

```cpp
ADD_API_METHOD_0(isUnlocked);
ADD_API_METHOD_0(loadKeyFile);
ADD_API_METHOD_1(setProductCheckFunction);
ADD_API_METHOD_1(writeKeyFile);
ADD_API_METHOD_0(getUserEmail);
ADD_API_METHOD_0(getRegisteredMachineId);
ADD_API_METHOD_1(isValidKeyFile);
ADD_API_METHOD_0(canExpire);
ADD_API_METHOD_1(checkExpirationData);
ADD_API_METHOD_0(keyFileExists);
ADD_API_METHOD_0(getLicenseKeyFile);
ADD_API_METHOD_1(contains);
ADD_API_METHOD_1(checkMuseHub);
ADD_API_METHOD_0(loadExpansionList);
ADD_API_METHOD_1(unlockExpansionList);
ADD_API_METHOD_1(writeExpansionKeyFile);
```

**No `addConstant()` calls** -- the constructor passes 0 to `ConstScriptingObject(p, 0)`.

## Wrapper Struct

```cpp
struct ScriptUnlocker::RefObject::Wrapper
{
    API_METHOD_WRAPPER_0(RefObject, isUnlocked);
    API_METHOD_WRAPPER_0(RefObject, canExpire);
    API_METHOD_WRAPPER_1(RefObject, checkExpirationData);
    API_METHOD_WRAPPER_0(RefObject, loadKeyFile);
    API_VOID_METHOD_WRAPPER_1(RefObject, setProductCheckFunction);
    API_METHOD_WRAPPER_1(RefObject, writeKeyFile);
    API_METHOD_WRAPPER_0(RefObject, getUserEmail);
    API_METHOD_WRAPPER_0(RefObject, getRegisteredMachineId);
    API_METHOD_WRAPPER_1(RefObject, isValidKeyFile);
    API_METHOD_WRAPPER_0(RefObject, keyFileExists);
    API_METHOD_WRAPPER_0(RefObject, getLicenseKeyFile);
    API_METHOD_WRAPPER_1(RefObject, contains);
    API_VOID_METHOD_WRAPPER_1(RefObject, checkMuseHub);
    API_METHOD_WRAPPER_0(RefObject, loadExpansionList);
    API_METHOD_WRAPPER_1(RefObject, unlockExpansionList);
    API_METHOD_WRAPPER_1(RefObject, writeExpansionKeyFile);
};
```

Note: `setProductCheckFunction` and `checkMuseHub` use `API_VOID_METHOD_WRAPPER` (void return).

## Preprocessor Guards

### USE_COPY_PROTECTION (default: 0)
Enables the copy protection system. When false, the `getLicenseUnlocker()` method is not available on `FrontendProcessor` at all. The `ScriptUnlocker` still exists in the backend unconditionally.

### USE_SCRIPT_COPY_PROTECTION (default: 0)
When true, automatically forces `USE_COPY_PROTECTION = 1`. Uses `ScriptUnlocker` as the `Unlocker` type in `FrontendProcessor`. When false (but `USE_COPY_PROTECTION` is true), a separate built-in `Unlocker` class with UI overlay is used instead.

### HISE_INCLUDE_UNLOCKER_OVERLAY
```cpp
#define HISE_INCLUDE_UNLOCKER_OVERLAY (USE_COPY_PROTECTION && !USE_SCRIPT_COPY_PROTECTION)
```
Controls whether the built-in registration UI overlay is shown. When `USE_SCRIPT_COPY_PROTECTION` is true, the overlay is disabled -- the script handles all UI.

### HISE_USE_UNLOCKER_FOR_EXPANSIONS (default: 0)
Enables expansion list management through the Unlocker. When true, the constructor auto-loads the expansion list, and methods `loadExpansionList`, `unlockExpansionList`, `writeExpansionKeyFile` become functional.

### HISE_INCLUDE_MUSEHUB
Guards the MuseHub SDK integration. When defined, `checkMuseHubInternal()` and the `ReferenceCountedObjectPtr<ReferenceCountedObject> m` member are available.

### JUCE_ALLOW_EXTERNAL_UNLOCK
Guards the `unlockExternal()` call in the backend's simulated MuseHub check.

## JUCE OnlineUnlockStatus Overrides

The `ScriptUnlocker` overrides the following virtual methods from `juce::OnlineUnlockStatus`:

### getProductID()
```cpp
String ScriptUnlocker::getProductID()
{
#if USE_BACKEND
    s << GET_HISE_SETTING(..., HiseSettings::Project::Name);
    s << " ";
    s << GET_HISE_SETTING(..., HiseSettings::Project::Version);
#else
    s << String(FrontendHandler::getProjectName());
    s << " ";
    s << FrontendHandler::getVersionString();
#endif
    return s;
}
```
Returns `"ProjectName Version"` string. Different sources for backend vs frontend.

### doesProductIDMatch(returnedIDFromServer)
```cpp
bool ScriptUnlocker::doesProductIDMatch(const String& returnedIDFromServer)
{
    // If a custom product check function is set, use it
    if (currentObject != nullptr && currentObject->pcheck)
    {
        var args(returnedIDFromServer);
        var rv(false);
        auto s = currentObject->pcheck.callSync(&args, 1, &rv);
        if (s.wasOk()) return rv;
    }
    // Default: compare product names WITHOUT version
    auto realId = getProductID().upToLastOccurrenceOf(" ", false, false).trim();
    auto expectedId = returnedIDFromServer.upToLastOccurrenceOf(" ", false, false).trim();
    return realId == expectedId;
}
```
**Important:** By default, version is stripped from the comparison. This means a key file generated for "MyPlugin 1.0" will still work with "MyPlugin 2.0". The `setProductCheckFunction` callback can override this behavior.

### getPublicKey()
Three compile-time variants:
1. `USE_BACKEND`: reads from `ProjectHandler::getPublicKey()` (the RSA key file in the project)
2. `!USE_COPY_PROTECTION || !USE_SCRIPT_COPY_PROTECTION`: returns empty `RSAKey()`
3. Implied third: when `USE_COPY_PROTECTION && USE_SCRIPT_COPY_PROTECTION`, it must be defined in the user's copy protection source file (not in HISE sources)

### saveState() / getState()
Both are no-ops (`jassertfalse` / empty string). The ScriptUnlocker does not use JUCE's built-in state persistence.

### getWebsiteName()
Returns `FrontendHandler::getCompanyWebsiteName()` in frontend; asserts in backend.

### getServerAuthenticationURL() / readReplyFromWebserver()
Both assert (`jassertfalse`) -- online authentication is not implemented through the JUCE mechanism. The script layer handles server communication directly.

## Key Infrastructure: Key File Format

The key file is a JUCE RSA-signed text file with this structure (inferred from parsing code):

```
Keyfile for ProductName
...
Machine numbers: XXXXX
...
[RSA-encrypted blob]
```

- `isValidKeyFile()` checks: `possibleKeyData.toString().startsWith("Keyfile for ")`
- `loadKeyFile()` parses "Machine numbers: " line to extract `registeredMachineId`
- `applyKeyFile(keyData)` is inherited from `juce::OnlineUnlockStatus` -- performs RSA validation

## Key File Location

```cpp
File ScriptUnlocker::getLicenseKeyFile()
{
#if USE_BACKEND
    // AppData/Company/Project/Project.extension
    return ProjectHandler::getAppDataRoot(mc)
        .getChildFile(company).getChildFile(project).getChildFile(project)
        .withFileExtension(FrontendHandler::getLicenseKeyExtension());
#else
    return FrontendHandler::getLicenseKey();
#endif
}
```

## Expansion List System

When `HISE_USE_UNLOCKER_FOR_EXPANSIONS` is enabled, the Unlocker manages expansion-level licensing.

### Expansion List File Location
```cpp
File ScriptUnlocker::getExpansionListFile()
{
    auto lf = getLicenseKeyFile();
    return lf.getSiblingFile("expansions").withFileExtension(lf.getFileExtension());
}
```
Sibling to the main license key file, named "expansions" with the same extension.

### getExpansionList() -- Decryption Flow
1. Read expansion list file, strip header before `#`
2. Parse hex string into `BigInteger`
3. Apply RSA public key decryption
4. Use registered machine ID as BlowFish key for second decryption layer
5. Parse resulting XML as `ValueTree` with type `"payload"`
6. Validate: email, machine_id, product must match
7. Extract expansion slug/key pairs into a `DynamicObject`

### loadExpansionList()
```cpp
bool ScriptUnlocker::RefObject::loadExpansionList()
{
    if (HISE_GET_PREPROCESSOR(..., HISE_USE_UNLOCKER_FOR_EXPANSIONS))
    {
        if(isUnlocked())
        {
            auto& h = getScriptProcessor()->getMainController_()->getExpansionHandler();
            h.setCredentials(unlocker->getExpansionList());
            return true;
        }
        return false;
    }
    else
        reportScriptError("HISE_USE_UNLOCKER_FOR_EXPANSIONS is not enabled");
}
```

### unlockExpansionList(expansionIdList)
**Backend-only** (`#if USE_BACKEND`). Scans expansion folders for metadata (either `project_info.xml` for FullInstrumentExpansion or `expansion_info.xml` for standard expansions), collects encryption keys, and passes them to `ExpansionHandler::setCredentials()`. Returns false in frontend builds.

### writeExpansionKeyFile(keyData)
Requires `HISE_USE_UNLOCKER_FOR_EXPANSIONS`. Validates that `keyData` starts with `"Expansion List"`, writes to the expansion list file, then calls `loadExpansionList()`.

## MuseHub Integration

### checkMuseHub(resultCallback) -- RefObject
```cpp
void ScriptUnlocker::RefObject::checkMuseHub(var resultCallback)
{
    if(unlocker.get() != nullptr)
    {
        mcheck = WeakCallbackHolder(getScriptProcessor(), this, resultCallback, 1);
        unlocker->checkMuseHub();
    }
}
```
Stores the callback in `mcheck` WeakCallbackHolder, then calls the parent's `checkMuseHub()`.

### checkMuseHub() -- ScriptUnlocker
```cpp
void ScriptUnlocker::checkMuseHub()
{
#if USE_BACKEND
    // SIMULATED: 50% random chance, delayed 2 seconds
    Timer::callAfterDelay(2000, [safeRef]()
    {
        auto ok = var(Random::getSystemRandom().nextFloat() > 0.5f);
        #if JUCE_ALLOW_EXTERNAL_UNLOCK
            if(ok) safeRef->unlockExternal();
        #endif
        if(safeRef != nullptr && safeRef->currentObject != nullptr)
            safeRef->currentObject->mcheck.call1(ok);
    });
#elif HISE_INCLUDE_MUSEHUB
    m = checkMuseHubInternal();
#endif
}
```
In backend: simulated with random result after 2s delay. In frontend with MuseHub SDK: calls internal implementation.

## Expiration System

### canExpire()
```cpp
var ScriptUnlocker::RefObject::canExpire() const
{
    return unlocker != nullptr ? var(unlocker->getExpiryTime() != juce::Time(0)) : var(false);
}
```
Returns true if the JUCE `OnlineUnlockStatus::getExpiryTime()` is non-zero.

### checkExpirationData(encodedTimeString)
```cpp
var ScriptUnlocker::RefObject::checkExpirationData(const String& encodedTimeString)
{
    if (unlocker != nullptr)
    {
        if (encodedTimeString.startsWith("0x"))
        {
            BigInteger bi;
            bi.parseString(encodedTimeString.substring(2), 16);
            unlocker->getPublicKey().applyToValue(bi);
            auto timeString = bi.toMemoryBlock().toString();
            auto time = Time::fromISO8601(timeString);
            auto ok = unlocker->unlockWithTime(time);
            auto delta = unlocker->getExpiryTime() - time;
            if (ok)
            {
                // In frontend, also reloads samples
                return var(roundToInt(delta.inDays()));  // returns days remaining
            }
            else
                return var(false);
        }
        return var("encodedTimeString data is corrupt");
    }
    return var("No unlocker");
}
```
Flow: hex string -> RSA decrypt -> ISO8601 time -> `unlockWithTime()`. Returns number of days remaining on success, false on failure, or error string.

## Copy Protection Macros

`HISE/hi_core/copyProtectionMacros.h` defines `CHECK_COPY_AND_RETURN_N(processor)` macros (1-24) that check `isUnlocked()` on the FrontendProcessor's unlocker. These are used throughout HISE modules to disable audio processing when unlocked status is undefined (not just false -- undefined specifically).

## RefObject Destructor

```cpp
ScriptUnlocker::RefObject::~RefObject()
{
    if (unlocker != nullptr && unlocker->currentObject == this)
        unlocker->currentObject = nullptr;
}
```
Clears the `currentObject` back-reference so the ScriptUnlocker doesn't call into a destroyed RefObject.

## WeakCallbackHolder Members

The RefObject has two `WeakCallbackHolder` members:
- `pcheck` -- product check callback, set via `setProductCheckFunction()`
- `mcheck` -- MuseHub result callback, set via `checkMuseHub()`

Both are initialized in the constructor with `(p, nullptr, var(), 1)` -- 1 argument slot.

## contains() Method

```cpp
bool ScriptUnlocker::RefObject::contains(String otherString)
{
    if(unlocker.get() != nullptr)
        return unlocker->contains(otherString);
    return true;
}
```
Delegates to `OnlineUnlockStatus::contains()` which is not defined on ScriptUnlocker -- this calls the JUCE base class method. The JUCE `OnlineUnlockStatus` doesn't have a public `contains()` method either; this likely refers to inherited `String` functionality on the key data or state. The method checks if the key file string contains the given substring. Returns true if unlocker is null (permissive fallback).

## Survey Data

- **domain:** preset-model
- **role:** service
- **creates:** File (via `getLicenseKeyFile()`)
- **references:** ExpansionHandler, FileSystem, File
- **seeAlso:**
  - BeatportManager: "Both handle product licensing/DRM but through different backends; Unlocker uses RSA key files while BeatportManager uses the Beatport SDK."
  - ExpansionHandler: "Unlocker can manage expansion-level licensing; ExpansionHandler manages the expansion packs themselves."
