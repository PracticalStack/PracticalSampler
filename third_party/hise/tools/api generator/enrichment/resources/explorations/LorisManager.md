# LorisManager -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` (LorisManager entry)
- `enrichment/base/LorisManager.json` (9 methods)
- No prerequisites from class_survey.md
- No base class explorations needed (not a component class)

## Class Declaration

**Header:** `HISE/hi_scripting/scripting/api/ScriptLorisManager.h`

```cpp
class ScriptLorisManager: public ConstScriptingObject,
                          public ControlledObject
{
public:
    ScriptLorisManager(ProcessorWithScriptingContent* p);
    Identifier getObjectName() const override { return "LorisManager"; }
    // ... 9 API methods ...
private:
    void initThreadController();
    uint32 lastTime;
    double progress = 0.0;
    ThreadController::Ptr scriptThreadController;
    struct Wrapper;
    WeakCallbackHolder logFunction;
    WeakCallbackHolder processFunction;
    LorisManager::Ptr lorisManager;
};
```

**Inheritance:**
- `ConstScriptingObject` -- standard scripting API base (read-only object name, API method registration)
- `ControlledObject` -- provides `getMainController()` access

**Core delegate:** `LorisManager` (in `hi_tools/hi_tools/LorisManager.h`) -- a `ReferenceCountedObject` that wraps the Loris C/C++ library via dynamic library loading or static linking.

## Constructor Analysis

**File:** `ScriptLorisManager.cpp:50-76`

```cpp
ScriptLorisManager::ScriptLorisManager(ProcessorWithScriptingContent* p):
  ConstScriptingObject(p, 0),    // 0 = no constants registered
  ControlledObject(p->getMainController_()),
  logFunction(p, nullptr, var(), 0),
  processFunction(p, nullptr, var(), 0)
{
    lorisManager = getMainController()->getLorisManager();
    
    if(lorisManager != nullptr)
    {
        lorisManager->setLogFunction([&](String m)
        {
            debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), m);
        });
    }
    
    ADD_API_METHOD_2(set);
    ADD_API_METHOD_1(get);
    ADD_API_METHOD_2(analyse);
    ADD_API_METHOD_1(synthesise);
    ADD_API_METHOD_3(process);
    ADD_API_METHOD_2(processCustom);
    ADD_API_METHOD_3(createEnvelopes);
    ADD_API_METHOD_3(createEnvelopePaths);
    ADD_API_METHOD_3(createSnapshot);
}
```

**Key observations:**
- Zero constants registered (`ConstScriptingObject(p, 0)`)
- ALL methods use plain `ADD_API_METHOD_N` -- no `ADD_TYPED_API_METHOD_N` registrations
- Log function is wired to console output via `debugToConsole`
- Grabs singleton `LorisManager` from `MainController`

## Method Wrappers

**File:** `ScriptLorisManager.cpp:36-48`

```cpp
struct ScriptLorisManager::Wrapper
{
    API_VOID_METHOD_WRAPPER_2(ScriptLorisManager, set);
    API_METHOD_WRAPPER_1(ScriptLorisManager, get);
    API_METHOD_WRAPPER_2(ScriptLorisManager, analyse);
    API_METHOD_WRAPPER_1(ScriptLorisManager, synthesise);
    API_VOID_METHOD_WRAPPER_3(ScriptLorisManager, process);
    API_VOID_METHOD_WRAPPER_2(ScriptLorisManager, processCustom);
    API_METHOD_WRAPPER_3(ScriptLorisManager, createEnvelopes);
    API_METHOD_WRAPPER_3(ScriptLorisManager, createEnvelopePaths);
    API_METHOD_WRAPPER_3(ScriptLorisManager, createSnapshot);
};
```

No typed wrappers. All use standard `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N`.

## Factory / obtainedVia

The `ScriptLorisManager` is created by `Engine.getLorisManager()`:

**File:** `ScriptingApi.cpp:2511-2518`
```cpp
juce::var ScriptingApi::Engine::getLorisManager()
{
#if HISE_INCLUDE_LORIS
    return var(new ScriptLorisManager(getScriptProcessor()));
#else
    return var();
#endif
}
```

Each call creates a NEW `ScriptLorisManager` wrapper, but they all share the same underlying `LorisManager` singleton from `MainController`. If `HISE_INCLUDE_LORIS` is not defined, returns undefined/empty var.

## Preprocessor Guards

**Primary guard:** `HISE_INCLUDE_LORIS`

This controls:
1. Whether `LorisManager.h` is included in `hi_tools.h` (line 232-234)
2. Whether `getLorisManager()` exists on `MainController` (MainController.h:2086-2088)
3. Whether `Engine.getLorisManager()` returns a real object or empty var (ScriptingApi.cpp:2513-2517)

**Secondary guard:** `HISE_USE_LORIS_DLL`

Inside `LorisManager.cpp`, there are two modes:
- `HISE_USE_LORIS_DLL`: Loads Loris as a dynamic library (`DynamicLibrary`) -- function pointers resolved at runtime
- `HISE_INCLUDE_LORIS` (without DLL): Static linking via `LorisLibrary::` namespace functions

The DLL path includes version checking (`HISE_LORIS_LIBRARY_MAJOR/MINOR/PATCH_VERSION` = 0.2.2).

## Core Delegate: LorisManager

**Header:** `HISE/hi_tools/hi_tools/LorisManager.h`

```cpp
struct LorisManager: public ReferenceCountedObject
{
    struct CustomPOD { ... };
    using Ptr = ReferenceCountedObjectPtr<LorisManager>;
    // ... function pointer typedefs ...
    struct AnalyseData { File file; double rootFrequency; };
    
    void* getFunction(const String& name) const;
    LorisManager(const File& hiseRoot_, const std::function<void(String)>& errorFunction_);
    ~LorisManager();
    
    // Core operations
    void analyse(const Array<AnalyseData>& data);
    Array<var> synthesise(const File& audioFile);
    bool process(const File& audioFile, String command, const String& jsonData);
    bool processCustom(const File& audioFile, const CustomPOD::Function& cf);
    Array<var> createEnvelope(const File& audioFile, const Identifier& parameter, int index);
    var getSnapshot(const File& f, double time, const Identifier& parameter);
    double get(String command) const;
    bool set(String command, String value);
    
    // Utilities
    Range<double> getEnvelopeRange(const Identifier& id) const;
    Path setEnvelope(const var& bf, const Identifier& id);
    bool checkError();
    void checkMessages();
    void setLogFunction(const std::function<void(String)>& logFunction);
    StringArray getList(bool getOptions);
    
    // Members
    char messageBuffer[2048];
    String lorisVersion;
    mutable ThreadController::Ptr threadController;
    std::function<void(String)> lf;
    std::function<void(String)> errorFunction;
    Result lastError;
    void* state = nullptr;
    ScopedPointer<DynamicLibrary> dll;
    File hiseRoot;
    CustomPOD::Function customFunction;
};
```

The `LorisManager` holds an opaque `state` pointer created by `createLorisState()` in the Loris library. All operations pass this state pointer through the C API.

## Loris C API (LorisLibrary)

**Header:** `HISE/hi_loris/wrapper/public.h`

The LorisLibrary struct provides a pure C API. Key functions exposed:

| C API Function | ScriptLorisManager method |
|----------------|--------------------------|
| `loris_analyze` | `analyse` |
| `loris_synthesize` | `synthesise` |
| `loris_process` | `process` |
| `loris_process_custom` | `processCustom` |
| `loris_set` | `set` |
| `loris_get` | `get` |
| `loris_create_envelope` | `createEnvelopes` |
| `loris_snapshot` | `createSnapshot` |
| `loris_prepare` | (not directly exposed -- called internally by `createSnapshot`) |

## CustomPOD Structure

**File:** `LorisManager.h:13-35`, `LorisManager.cpp:53-84`

The `CustomPOD` is the data structure passed to the `processCustom` callback:

**Read-only fields (constants):**
| Property | Type | Description |
|----------|------|-------------|
| channelIndex | int | Channel in the audio file |
| partialIndex | int | Index of the partial |
| sampleRate | double | Sample rate of the file |
| rootFrequency | double | Root frequency passed to analyse |

**Read-write fields (variable properties):**
| Property | Type | Description |
|----------|------|-------------|
| time | double | Time of the breakpoint |
| frequency | double | Frequency of the partial at the breakpoint (Hz) |
| phase | double | Phase in radians (0..2*PI) |
| gain | double | Amplitude of the partial |
| bandwidth | double | Noisiness (0.0 = pure sine, 1.0 = full noise) |

`toJSON()` creates a DynamicObject with all properties.
`writeJSON()` reads back only the variable properties (time, frequency, phase, gain, bandwidth) -- the constants are commented out and not written back.

## Threading / initThreadController

**File:** `ScriptLorisManager.cpp:182-195`

```cpp
void ScriptLorisManager::initThreadController()
{
    if(lorisManager == nullptr)
        reportScriptError("Loris is not available");

    if(scriptThreadController == nullptr && Thread::getCurrentThread() != nullptr)
    {
        scriptThreadController = new ThreadController(Thread::getCurrentThread(), &progress, 500, lastTime);
    }

    lorisManager->threadController = scriptThreadController;
    progress = 0.0;
}
```

Every public method calls `initThreadController()` before delegating. This:
1. Checks Loris availability (throws script error if null)
2. Creates a `ThreadController` from the current thread (if on a background thread)
3. Assigns it to the `lorisManager->threadController` for progress tracking

The thread controller enables cancellation and progress reporting during long-running Loris operations. `Thread::getCurrentThread()` returns nullptr on the message thread, so the ThreadController is only created when running from a background thread (e.g., via `BackgroundTask`).

## OptionIds (for `set`/`get`)

**File:** `HISE/hi_loris/wrapper/Properties.h:25-43`

| Option ID | Type | Default | Description |
|-----------|------|---------|-------------|
| timedomain | String | "seconds" | Time axis domain: "seconds", "samples", or "0to1" |
| enablecache | bool | true | Cache analysed partials for reuse |
| windowwidth | double | 1.0 | Window width scale factor (clamped 0.125..4.0) |
| freqfloor | double | 40.0 | Lowest frequency considered harmonic content (Hz) |
| ampfloor | double | 90.0 | Lowest amplitude above noise floor (dB) |
| sidelobes | double | 90.0 | Side lobe gain of analysis window (dB) |
| freqdrift | double | 50.0 | Max frequency drift tolerance (cents) |
| hoptime | double | 0.0129 | Time between analysis windows (seconds) |
| croptime | double | 0.0129 | Crop time parameter (seconds) |
| bwregionwidth | double | 1.0 | Bandwidth region width |

**Behavioral notes from Options::update() (Helpers.cpp:68-96):**
- `timedomain`: validated against string list, throws on invalid value
- `freqdrift`: stored but does NOT call analyzer setter (unlike others)
- `windowwidth`: clamped via `jlimit(0.125, 4.0, value)`
- `enablecache`: boolean, controls whether re-analysis is skipped for same file
- Most numeric options call corresponding `analyzer_setXXX()` functions when `initialised` is true
- Unknown option ID throws: `"Invalid option: " + id`

The `get()` method on LorisManager calls `loris_get` which returns a double. The `set()` method calls `loris_set` which takes two strings (command, value).

## ProcessIds (for `process` command parameter)

**File:** `HISE/hi_loris/wrapper/Properties.h:60-77`

| Command | JSON Data Format | Description |
|---------|-----------------|-------------|
| reset | `{}` (empty object) | Resets partials to original analysis state |
| shiftTime | `{"offset": number}` | Shifts all partial times by offset |
| shiftPitch | `{"offset": number}` or `[[time,val],...]` array | Shifts pitch by constant or envelope |
| scaleFrequency | array (envelope points) | Scales frequency by envelope |
| dilate | `[[inputTimes], [targetTimes]]` | Time-stretches using input/target time pairs |
| applyFilter | array (envelope points in frequency domain) | Applies gain envelope in frequency domain |
| custom | (not used via `process` -- use `processCustom` instead) | Custom processing placeholder |

**Behavioral tracing from MultichannelPartialList::process() (MultichannelPartialList.cpp:173-310):**

- `reset`: Copies `original` partial list back to `list` -- requires empty JSON object
- `shiftTime`: Requires `{"offset": value}` -- offset is converted to seconds via `convertTimeToSeconds`
- `shiftPitch`: Two modes:
  - Array of envelope points: creates LinearEnvelope, applies via `shiftPitch()`
  - Object with `{"offset": value}`: creates constant envelope at that offset
- `scaleFrequency`: Creates envelope from JSON array, applies via `scaleFrequency()`
- `dilate`: Requires array of two arrays: `[inputTimes, targetTimes]` -- both must be arrays of doubles
- `applyFilter`: Temporarily switches to Frequency time domain, creates envelope from JSON, multiplies amplitude by envelope value at each partial's frequency
- Invalid command throws: `"Invalid command: " + command`

## ParameterIds (for envelope/snapshot parameter strings)

**File:** `HISE/hi_loris/wrapper/Properties.h:45-57`

| Parameter | Description | Envelope Range |
|-----------|-------------|----------------|
| rootFrequency | F0 estimate relative to root | freqdrift-derived (centered on 1.0) |
| frequency | Partial frequency in Hz | freqdrift-derived (centered on 1.0) |
| phase | Phase in radians | -PI to PI |
| gain | Amplitude | 0.0 to 1.0 |
| bandwidth | Noisiness | 0.0 to 1.0 |

Envelope ranges are computed in `LorisManager::getEnvelopeRange()` (LorisManager.cpp:490-519):
- For frequency/rootFrequency: range is `[1/d, d]` where `d = 2^(freqdrift/1200)`
- For gain: `[0, 1]`
- For phase: `[-PI, PI]`
- For bandwidth: `[0, 1]`

The `rootFrequency` parameter is special -- it uses `createF0Estimate` internally rather than reading from individual partial labels. All other parameters require `prepareToMorph()` to be called first (which channelizes, collates, sifts, distills, and sorts the partial list).

## processCustom Flow

**File:** `ScriptLorisManager.cpp:112-133`

```cpp
void ScriptLorisManager::processCustom(var file, var processCallback)
{
    initThreadController();
    processFunction = WeakCallbackHolder(getScriptProcessor(), this, processCallback, 1);

    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        lorisManager->processCustom(sf->f, [&](LorisManager::CustomPOD& data)
        {
            auto obj = data.toJSON();
            auto ok = processFunction.callSync(&obj, 1);
            
            if(!ok.wasOk())
                reportScriptError(ok.getErrorMessage());
            
            data.writeJSON(obj);
            return false;
        });
    }
}
```

Flow:
1. Creates a `WeakCallbackHolder` wrapping the script function (1 argument)
2. For each breakpoint in the partial list, the Loris library calls the lambda
3. Lambda converts `CustomPOD` to JSON, calls script function synchronously, writes modified JSON back
4. The callback always returns `false` (continue iterating)
5. Script function receives a JSON object with all CustomPOD properties
6. Script function can modify the mutable properties (time, frequency, phase, gain, bandwidth)

## createEnvelopePaths Flow

**File:** `ScriptLorisManager.cpp:145-170`

This method calls `createEnvelopes` internally, then converts each buffer to a `Path` object:
1. Calls `createEnvelopes` to get array of VariantBuffers
2. For each buffer, calls `lorisManager->setEnvelope()` which creates a JUCE Path
3. Wraps each Path in a `ScriptingObjects::PathObject`
4. Returns array of PathObjects

The Path creation in `setEnvelope()` (LorisManager.cpp:521-570) downsamples the buffer (3 * size / 600 samples per pixel) and clips values outside the valid range for the parameter.

## MainController Lifecycle

**File:** `MainController.h:2087,2135`

```cpp
#if HISE_INCLUDE_LORIS
    LorisManager* getLorisManager() { return lorisManager.get(); }
#endif
    // ...
    LorisManager::Ptr lorisManager;
```

**Backend creation:** `BackendProcessor.cpp:522-536`
```cpp
lorisManager = new LorisManager(File(), [this](String message)
{ ... });
// Later, with hise root:
lorisManager = new LorisManager(f, [this](String message)
{ ... });
```

**Frontend creation:** `FrontEndProcessor.cpp:272`
```cpp
lorisManager = new LorisManager(f, [this](String m)
```

Both backend and frontend create a LorisManager. The `hiseRoot` parameter is used to locate the Loris DLL when using `HISE_USE_LORIS_DLL` mode.

## File Parameter Pattern

All methods that accept a `var file` parameter expect a `ScriptingObjects::ScriptFile` object (obtained from `FileSystem.fromAbsolutePath()` or similar). The pattern is:
```cpp
if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    // use sf->f (juce::File member)
```

If the cast fails, methods return false/empty without error.

## Error Handling

Two error paths:
1. **Loris not available:** `initThreadController()` calls `reportScriptError("Loris is not available")` -- this is a hard script error
2. **Loris library errors:** After each Loris C API call, `checkError()` is called which retrieves the last error string via `getLastError()`. If non-empty, it's reported via the `errorFunction` callback (which goes to the console)
3. **Process command errors:** Invalid commands throw `Result::fail` in the Loris library

## Version Information

The library version is `HISE_LORIS_LIBRARY_MAJOR/MINOR/PATCH_VERSION` = 0.2.2. On DLL load, the version is checked against the compiled version and a mismatch triggers an error.
