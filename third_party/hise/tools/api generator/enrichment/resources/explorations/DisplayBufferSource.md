# DisplayBufferSource -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- no prerequisite listed for DisplayBufferSource
- `enrichment/resources/survey/class_survey_data.json` -- no entry found (class not in survey data)
- `enrichment/base/DisplayBufferSource.json` -- 1 API method (getDisplayBuffer)
- `enrichment/resources/explorations/DisplayBuffer.md` -- related class exploration (DisplayBuffer is the object returned by this class)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 2531

```cpp
class ScriptDisplayBufferSource : public ConstScriptingObject
{
public:
    ScriptDisplayBufferSource(ProcessorWithScriptingContent *p, ProcessorWithExternalData *h);
    ~ScriptDisplayBufferSource() {};

    // =============================================================================================

    /** Returns a reference to the display buffer at the given index. */
    var getDisplayBuffer(int index);

    // =============================================================================================

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("DisplayBufferSource"); }

    Identifier getObjectName() const override { return getClassName(); };
    bool objectDeleted() const override { return source.get() == nullptr; }
    bool objectExists() const override { return source != nullptr; }

private:
    struct Wrapper;
    WeakReference<ExternalDataHolder> source;
};
```

**Key observations:**
- C++ class name is `ScriptDisplayBufferSource`, scripting API name is `DisplayBufferSource`
- Inherits from `ConstScriptingObject` directly (NOT from `ScriptComplexDataReferenceBase`)
- Holds a `WeakReference<ExternalDataHolder>` to the source processor
- The source is typed as `ProcessorWithExternalData` in the constructor but stored as `ExternalDataHolder` (its base class)
- Has only one API method: `getDisplayBuffer(int index)`
- Object validity is tracked via the weak reference (objectDeleted/objectExists)
- This is a very thin wrapper -- its sole purpose is to provide access to the `DisplayBuffer` objects owned by a processor

## Constructor and Method Registration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 7302

```cpp
struct ScriptingObjects::ScriptDisplayBufferSource::Wrapper
{
    API_METHOD_WRAPPER_1(ScriptDisplayBufferSource, getDisplayBuffer);
};

ScriptingObjects::ScriptDisplayBufferSource::ScriptDisplayBufferSource(
    ProcessorWithScriptingContent *p, ProcessorWithExternalData *h):
    ConstScriptingObject(p, 0),  // 0 constants
    source(h)
{
    ADD_API_METHOD_1(getDisplayBuffer);
}
```

**Key observations:**
- Constructor passes `0` to `ConstScriptingObject` -- zero constants registered
- Uses `ADD_API_METHOD_1` (NOT `ADD_TYPED_API_METHOD_1`) -- no forced parameter types
- Uses `API_METHOD_WRAPPER_1` (not void) -- returns a value
- The `ProcessorWithExternalData *h` is stored as `WeakReference<ExternalDataHolder> source`

## getDisplayBuffer Implementation

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 7309

```cpp
var ScriptingObjects::ScriptDisplayBufferSource::getDisplayBuffer(int index)
{
    if (objectExists())
    {
        auto numObjects = source->getNumDataObjects(ExternalData::DataType::DisplayBuffer);

        if (isPositiveAndBelow(index, numObjects))
            return var(new ScriptingObjects::ScriptRingBuffer(
                getScriptProcessor(), index,
                dynamic_cast<ProcessorWithExternalData*>(source.get())));

        reportScriptError("Can't find buffer at index " + String(index));
    }

    RETURN_IF_NO_THROW({});
}
```

**Key observations:**
- Validates the index against the number of display buffers on the source processor
- Creates and returns a new `ScriptRingBuffer` (scripting name: `DisplayBuffer`) for the requested index
- Error cases: source is null (objectExists fails silently), index out of range (reportScriptError)
- The returned `ScriptRingBuffer` receives a pointer to the same `ProcessorWithExternalData` as its holder

## Factory Method (obtainedVia)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 6169

```cpp
hise::ScriptingObjects::ScriptDisplayBufferSource* ScriptingApi::Synth::getDisplayBufferSource(const String& name)
{
    using namespace snex;

    WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

    if (getScriptProcessor()->objectsCanBeCreated())
    {
        Processor::Iterator<ProcessorWithExternalData> it(owner);

        while (auto eh = it.getNextProcessor())
        {
            if (dynamic_cast<Processor*>(eh)->getId() == name)
            {
                if (eh->getNumDataObjects(ExternalData::DataType::DisplayBuffer) > 0)
                    return new ScriptingObjects::ScriptDisplayBufferSource(getScriptProcessor(), eh);
                else
                    reportScriptError("No display buffer available");
            }
        }

        reportScriptError(name + " was not found. ");
        RETURN_IF_NO_THROW(new ScriptingObjects::ScriptDisplayBufferSource(getScriptProcessor(), nullptr));
    }
    else
    {
        reportIllegalCall("getScriptingTableProcessor()", "onInit");
        RETURN_IF_NO_THROW(new ScriptingObjects::ScriptDisplayBufferSource(getScriptProcessor(), nullptr));
    }
}
```

**Key observations:**
- Created via `Synth.getDisplayBufferSource(name)` -- the name is the processor ID
- **onInit-only** -- calls `objectsCanBeCreated()` and reports illegal call if not in onInit
- Uses `Processor::Iterator<ProcessorWithExternalData>` to search the module tree for a processor matching the given name
- Validates that the found processor actually has at least one DisplayBuffer before returning
- Error in the illegal-call message says "getScriptingTableProcessor()" -- this is a copy-paste artifact, not the actual method name
- `WARN_IF_AUDIO_THREAD` guards against audio thread creation

## ProcessorWithExternalData Infrastructure

**File:** `HISE/hi_core/hi_dsp/ProcessorInterfaces.h`, line 40

`ProcessorWithExternalData` extends `ExternalDataHolder` (defined in `hi_dsp_library/snex_basics/snex_ExternalData.h`). The `ExternalDataHolder` interface defines:

```cpp
struct ExternalDataHolder
{
    virtual int getNumDataObjects(ExternalData::DataType t) const = 0;
    virtual Table* getTable(int index) = 0;
    virtual SliderPackData* getSliderPack(int index) = 0;
    virtual MultiChannelAudioBuffer* getAudioFile(int index) = 0;
    virtual FilterDataObject* getFilterData(int index) = 0;
    virtual SimpleRingBuffer* getDisplayBuffer(int index) = 0;
    virtual bool removeDataObject(ExternalData::DataType t, int index) = 0;
    // ...
};
```

Three concrete subclasses provide different data storage strategies:
- `ProcessorWithSingleStaticExternalData` -- single data type, fixed count
- `ProcessorWithStaticExternalData` -- multiple data types, fixed counts
- `ProcessorWithDynamicExternalData` -- multiple data types, resizable

The `ExternalData::DataType` enum includes:
- `Table` -- lookup table (512 floats)
- `SliderPack` -- resizable float array
- `AudioFile` -- multichannel audio with metadata
- `FilterCoefficients` -- for display purposes
- `DisplayBuffer` -- FIFO ring buffer for analysis/visualization

## ProcessorMetadata Interface Registration

**File:** `HISE/hi_core/hi_dsp/ProcessorMetadata.h`, line 745-746

```cpp
if (dt == ExternalData::DataType::DisplayBuffer)
    copy.interfaceClasses.add(ProcessorMetadataIds::DisplayBufferSource);
```

Processors that have display buffers are automatically tagged with the `DisplayBufferSource` interface class in the metadata system. This means `Synth.getDisplayBufferSource()` can find any processor in the module tree that has at least one `DisplayBuffer` slot -- the interface class in ProcessorMetadata enables this discovery.

The `DisplayBufferSource` identifier is declared at line 82:
```cpp
DECLARE_ID(DisplayBufferSource);
```

## Relationship to DisplayBuffer (ScriptRingBuffer)

`DisplayBufferSource` is a thin intermediary between the module tree and `DisplayBuffer` objects:

1. **Synth.getDisplayBufferSource(name)** -- locates a processor by name, wraps it as `DisplayBufferSource`
2. **DisplayBufferSource.getDisplayBuffer(index)** -- retrieves the Nth display buffer from that processor, returns a `DisplayBuffer` object

The `DisplayBuffer` (C++ class `ScriptRingBuffer`) is the actual data handle that provides:
- `getReadBuffer()` -- read the current buffer contents
- `getResizedBuffer()` -- resampled version
- `createPath()` -- vector path for visualization
- `setRingBufferProperties()` -- configure buffer behavior
- `copyReadBuffer()` -- copy to preallocated buffer
- `setActive()` -- enable/disable
- `toBase64()` / `fromBase64()` -- serialization

## Which Processors Have Display Buffers

Any processor implementing `ProcessorWithExternalData` that declares display buffer slots. Common examples include:
- Hardcoded modules (via `HardcodedModuleBase` which inherits `ProcessorWithExternalData`)
- Scriptnode networks (via `DspNetwork` which manages external data)
- Any processor using `ProcessorWithStaticExternalData` or `ProcessorWithDynamicExternalData` with display buffer slots

## Threading and Lifecycle Constraints

- **onInit-only creation:** `Synth.getDisplayBufferSource()` checks `objectsCanBeCreated()` and fails outside onInit
- **Audio thread guard:** `WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation)` prevents creation on the audio thread
- **Weak reference safety:** The source is held via `WeakReference<ExternalDataHolder>`, so the wrapper gracefully handles processor deletion (objectExists/objectDeleted checks)
- **No preprocessor guards:** No `USE_BACKEND` or other conditional compilation

## Constants

None. Constructor passes `0` to `ConstScriptingObject`, and no `addConstant()` calls are made.
