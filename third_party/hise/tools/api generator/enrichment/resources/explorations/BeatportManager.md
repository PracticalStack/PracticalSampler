# BeatportManager -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- BeatportManager entry
- `enrichment/resources/survey/class_tags.json` -- group: services, role: service
- `enrichment/phase1/Engine/Readme.md` -- prerequisite class (factory owner)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptExpansion.h`, line 674

```cpp
/** A wrapper around the beatport authentication system. */
class BeatportManager: public ConstScriptingObject
{
public:
    BeatportManager(ProcessorWithScriptingContent* sp);
    ~BeatportManager();

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("BeatportManager"); }

    // API Methods
    void setProductId(const String& productId);
    var validate();
    bool isBeatportAccess();

    // Static utility
    static File getBeatportProjectFolder(MainController* mc);

private:
#if HISE_INCLUDE_BEATPORT
    struct Pimpl
    {
        Pimpl();
        ~Pimpl();
        void setProductId(const String& productId);
        var validate();
        bool isBeatportAccess() const;
    private:
        struct Data;
        Data* data;  // opaque pointer to SDK internals
    };
    ScopedPointer<Pimpl> pimpl;
#endif

    struct Wrapper;
    JUCE_DECLARE_WEAK_REFERENCEABLE(BeatportManager);
};
```

### Inheritance

- `ConstScriptingObject` -- standard base for immutable scripting API objects
- No listener interfaces, no additional inheritance

### Inner Types

- `Pimpl` -- private implementation struct, only compiled when `HISE_INCLUDE_BEATPORT` is defined. Uses a raw `Data*` pointer (opaque pimpl pattern) to isolate the Beatport SDK headers from the rest of HISE.
- `Wrapper` -- standard API method wrapper struct

## Preprocessor Guards

### HISE_INCLUDE_BEATPORT

**Defined in:** `HISE/hi_core/hi_core.h`, line 540-541

```cpp
#ifndef HISE_INCLUDE_BEATPORT
#define HISE_INCLUDE_BEATPORT 0
#endif
```

**Default: 0 (disabled).** This is an opt-in SDK integration. When disabled:
- The `Pimpl` struct and its `ScopedPointer<Pimpl> pimpl` member are not compiled
- All three API methods have fallback behavior (see below)

This preprocessor flag controls the entire class's real functionality. Without it, BeatportManager operates in a simulation/development mode.

## Constructor

**File:** `ScriptExpansion.cpp`, line 3424-3434

```cpp
BeatportManager::BeatportManager(ProcessorWithScriptingContent* sp):
    ConstScriptingObject(sp, 0)  // 0 = no constants
{
#if HISE_INCLUDE_BEATPORT
    pimpl = new Pimpl();
#endif

    ADD_API_METHOD_0(validate);
    ADD_API_METHOD_0(isBeatportAccess);
    ADD_API_METHOD_1(setProductId);
}
```

Key observations:
- **Zero constants** -- `ConstScriptingObject(sp, 0)` passes 0 for numConstants
- No `addConstant()` calls
- No `ADD_TYPED_API_METHOD_N` calls -- all use plain `ADD_API_METHOD_N`
- Pimpl created only when Beatport SDK is available

## Factory / obtainedVia

Created by `Engine.createBeatportManager()`:

**File:** `ScriptingApi.cpp`, line 2184-2187

```cpp
var ScriptingApi::Engine::createBeatportManager()
{
    return var(new BeatportManager(getScriptProcessor()));
}
```

Registered in Engine constructor at line 1417:
```cpp
ADD_API_METHOD_0(createBeatportManager);
```

The API wrapper at line 1210:
```cpp
API_METHOD_WRAPPER_0(Engine, createBeatportManager);
```

## API Method Wrapper Registration

**File:** `ScriptExpansion.cpp`, line 3417-3422

```cpp
struct BeatportManager::Wrapper
{
    API_METHOD_WRAPPER_0(BeatportManager, validate);
    API_METHOD_WRAPPER_0(BeatportManager, isBeatportAccess);
    API_VOID_METHOD_WRAPPER_1(BeatportManager, setProductId);
};
```

All three methods use plain (untyped) wrappers. No typed variants.

## Method Implementations -- Infrastructure Context

### setProductId (line 3443-3449)

Two code paths:
- **HISE_INCLUDE_BEATPORT=1:** Delegates to `pimpl->setProductId(productId)` -- passes to SDK
- **HISE_INCLUDE_BEATPORT=0:** Logs to console via `debugToConsole()` -- development stub

### validate (line 3452-3483)

Two code paths:
- **HISE_INCLUDE_BEATPORT=1:** Delegates to `pimpl->validate()` which calls the real SDK
- **HISE_INCLUDE_BEATPORT=0 (simulation mode):**
  1. Simulates network latency with `Thread::getCurrentThread()->wait(1500)` (1.5 seconds)
  2. Reads `validate_response.json` from the beatport project folder
  3. Reports script error if file missing or JSON parse fails
  4. Returns parsed JSON object

**Important threading detail:** After both code paths, the method calls:
```cpp
dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout((int)(now - t));
```
This extends the script execution timeout by the elapsed wall-clock time to prevent the synchronous wait from triggering the script watchdog timer. This is a blocking call that runs on the script compilation thread.

### isBeatportAccess (line 3485-3501)

Two code paths:
- **HISE_INCLUDE_BEATPORT=1:** Delegates to `pimpl->isBeatportAccess()`
- **HISE_INCLUDE_BEATPORT=0 (simulation mode):**
  1. Waits 500ms via `Thread::getCurrentThread()->wait(500)`
  2. Extends script timeout (same pattern as validate)
  3. Returns `true` if `validate_response.json` exists in the beatport folder

### getBeatportProjectFolder (static, line 698-713, in header)

Backend-only static utility:
- **USE_BACKEND=1:** Returns `{project}/AdditionalSourceCode/beatport/`, creates directory if missing
- **USE_BACKEND=0:** Asserts false and returns empty File -- should never be called in compiled plugins

This is NOT registered as a scripting API method but is used internally by the simulation code paths.

Note: The base JSON lists this as an API method with `MainController*` parameter, but it is a static C++ utility, not exposed to HiseScript. It appears in the Doxygen-generated API reference but is not callable from scripts.

## Pimpl Pattern -- SDK Isolation

The `Pimpl` struct uses a double-indirection pattern:
1. `BeatportManager` holds `ScopedPointer<Pimpl> pimpl`
2. `Pimpl` holds raw `Data* data` with forward-declared `struct Data`

This isolates the Beatport SDK headers completely. The `Data` struct definition is not in any scanned HISE header -- it would be in a separate .cpp file that includes the Beatport SDK headers. This means the actual SDK integration code is not available for analysis.

## Simulation Mode -- Development Workflow

When `HISE_INCLUDE_BEATPORT=0` (default), the class operates as a development/testing stub:

1. Developer creates `{project}/AdditionalSourceCode/beatport/validate_response.json` with a mock response
2. `validate()` reads and parses this file, simulating a 1.5s network delay
3. `isBeatportAccess()` checks if the file exists, simulating a 0.5s check
4. `setProductId()` just logs to console

This allows scripting the DRM flow without having the actual Beatport SDK installed.

## Threading and Lifecycle

- **Blocking calls:** Both `validate()` and `isBeatportAccess()` are synchronous and blocking. They use `Thread::getCurrentThread()->wait()` in simulation mode, and presumably the real SDK calls are also synchronous.
- **Timeout extension:** Both methods extend the script engine timeout to compensate for their blocking nature. This is critical -- without it, the script watchdog would kill the script during the wait.
- **No thread safety concerns:** The class has no shared mutable state beyond the pimpl. It is created and used from the scripting thread.
- **No onInit restriction:** Can be created at any time, though typically created during initialization.

## Upstream Data Providers

The class is self-contained in terms of data flow:
- **Input:** Product ID string (from script), Beatport SDK (from compiled library)
- **Output:** Validation result JSON, boolean access check
- **Simulation input:** `validate_response.json` file on disk
- The `MainController` is used only to locate the project folder (via `getBeatportProjectFolder`)

## Relationship to Engine Prerequisite

BeatportManager is created by `Engine.createBeatportManager()`. It is one of Engine's factory products (see Engine Readme, "Object Factories" section). The Engine prerequisite describes it as part of the `create*` factory pattern alongside `createBXLicenser()` -- both are SDK-gated DRM integrations.

## Survey Data Summary

- **Domain:** preset-model
- **Role:** service
- **createdBy:** Engine
- **creates:** nothing
- **seeAlso:** Unlocker (both handle DRM/licensing; BeatportManager uses Beatport SDK, Unlocker uses RSA key file validation)
- **callbackDensity:** 0.0 (no callbacks)
- **statefulness:** 0.67 (moderate -- holds product ID and SDK state)
- **threadingExposure:** 0.2 (low -- blocking but self-contained)
