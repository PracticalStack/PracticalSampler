# SliderPackProcessor -- C++ Source Exploration

## Resources Consulted

- `enrichment/base/SliderPackProcessor.json` -- base API definition (1 method)
- `enrichment/resources/survey/class_survey.md` -- prerequisites table (no prerequisite listed for SliderPackProcessor)
- No entry found in `class_survey_data.json` for SliderPackProcessor

## Class Overview

SliderPackProcessor is the scripting API name for `ScriptingObjects::ScriptSliderPackProcessor`, a thin wrapper that provides access to SliderPack data owned by audio modules. It has a single method: `getSliderPack(int sliderPackIndex)`.

## C++ Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` line 2500

```cpp
class ScriptSliderPackProcessor : public ConstScriptingObject
{
public:
    ScriptSliderPackProcessor(ProcessorWithScriptingContent* p, ExternalDataHolder* h);

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("SliderPackProcessor"); }

    Identifier getObjectName() const override { return getClassName(); };
    bool objectDeleted() const override { return sp.get() == nullptr; }
    bool objectExists() const override { return sp.get() != nullptr; }

    /** Creates a data reference to the given index. */
    var getSliderPack(int sliderPackIndex);

private:
    struct Wrapper;
    WeakReference<Processor> sp;
};
```

Key observations:
- Extends `ConstScriptingObject` (0 constants in constructor)
- Stores a `WeakReference<Processor>` to the target module -- the `ExternalDataHolder*` is `dynamic_cast` to `Processor*` at construction
- Object validity checks use the WeakReference (`sp.get() == nullptr`)
- No constants, no typed API methods

## Constructor

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` line 5351

```cpp
ScriptingObjects::ScriptSliderPackProcessor::ScriptSliderPackProcessor(
    ProcessorWithScriptingContent* p, ExternalDataHolder* h) :
    ConstScriptingObject(p, 0),   // 0 = no constants
    sp(dynamic_cast<Processor*>(h))
{
    ADD_API_METHOD_1(getSliderPack);
}
```

- Uses `ADD_API_METHOD_1` (not typed), so `getSliderPack` has no forced parameter types
- The `ExternalDataHolder*` is cast to `Processor*` for the WeakReference storage
- The constructor takes `ExternalDataHolder*`, not `SliderPackProcessor*` -- this means it can wrap ANY module that implements ExternalDataHolder, not just modules that inherit from the C++ `SliderPackProcessor` class

## Method Registration (Wrapper)

```cpp
struct ScriptingObjects::ScriptSliderPackProcessor::Wrapper
{
    API_METHOD_WRAPPER_1(ScriptSliderPackProcessor, getSliderPack);
};
```

Uses `API_METHOD_WRAPPER_1` (non-void return, 1 parameter). No typed variants.

## getSliderPack Implementation

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` line 5358

```cpp
var ScriptingObjects::ScriptSliderPackProcessor::getSliderPack(int sliderPackIndex)
{
    if (checkValidObject())
    {
        if (auto ed = dynamic_cast<ProcessorWithExternalData*>(sp.get()))
            return var(new ScriptSliderPackData(getScriptProcessor(), sliderPackIndex, ed));
    }

    reportScriptError("Not a valid object");
    RETURN_IF_NO_THROW(var());
}
```

Key behavior:
1. Validates the object is still alive via `checkValidObject()` (inherited from ConstScriptingObject)
2. Casts the stored `Processor*` back to `ProcessorWithExternalData*`
3. Creates a `ScriptSliderPackData` object passing the index and the external data holder
4. The returned `ScriptSliderPackData` is the actual data handle (class name "SliderPackData" in HiseScript)
5. No bounds checking on `sliderPackIndex` at this level -- that is handled inside `ScriptSliderPackData` / `ProcessorWithSingleStaticExternalData`

## Factory Method (obtainedVia)

SliderPackProcessor instances are created by `Synth.getSliderPackProcessor(name)`.

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` line 6143

```cpp
hise::ScriptingApi::Synth::ScriptSliderPackProcessor* ScriptingApi::Synth::getSliderPackProcessor(const String& name)
{
    WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

    if (getScriptProcessor()->objectsCanBeCreated())
    {
        Processor::Iterator<ExternalDataHolder> it(owner);

        while (auto sp = it.getNextProcessor())
        {
            if (dynamic_cast<Processor*>(sp)->getId() == name)
            {
                return new ScriptSliderPackProcessor(getScriptProcessor(), sp);
            }
        }

        reportScriptError(name + " was not found. ");
        RETURN_IF_NO_THROW(new ScriptSliderPackProcessor(getScriptProcessor(), nullptr));
    }
    else
    {
        reportIllegalCall("getSliderPackProcessor()", "onInit");
        RETURN_IF_NO_THROW(new ScriptSliderPackProcessor(getScriptProcessor(), nullptr));
    }
}
```

Key observations:
1. **onInit-only restriction:** Uses `objectsCanBeCreated()` guard -- must be called in `onInit`
2. **Audio thread warning:** `WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation)` -- cannot be called from audio thread
3. **Iteration target:** Uses `Processor::Iterator<ExternalDataHolder>` -- iterates ALL modules that implement `ExternalDataHolder`, not just `SliderPackProcessor` subclasses
4. **Name matching:** Matches by processor ID (`getId() == name`)
5. **Error on not found:** Reports script error if no module with that name is found

Registration in Synth constructor:
```cpp
ADD_API_METHOD_1(getSliderPackProcessor);  CHECK_MODULE(getSliderPackProcessor, ProcessorWithExternalData);
```

The `CHECK_MODULE` macro adds a diagnostic that verifies the module name resolves to a `ProcessorWithExternalData` at parse time.

## Inheritance Chain

```
ExternalDataHolder (snex namespace, pure interface)
  -> ProcessorWithExternalData (adds MainController, shared references, UI updater)
    -> ProcessorWithSingleStaticExternalData (single data type, N objects)
      -> SliderPackProcessor (C++ class, specializes for SliderPack data type)
```

The C++ `SliderPackProcessor` class:
- **File:** `HISE/hi_core/hi_dsp/ProcessorInterfaces.h` line 293
- Inherits from `ProcessorWithSingleStaticExternalData`
- Constructor passes `ExternalData::DataType::SliderPack` as the data type
- Adds `getSliderPackUnchecked(int index)` convenience accessor
- Very thin -- just a type-specialized construction helper

```cpp
class SliderPackProcessor: public ProcessorWithSingleStaticExternalData
{
public:
    SliderPackProcessor(MainController* mc, int numSliderPacks);
    SliderPackData* getSliderPackUnchecked(int index = 0);
    const SliderPackData* getSliderPackUnchecked(int index = 0) const;
};
```

Constructor implementation:
```cpp
SliderPackProcessor::SliderPackProcessor(MainController* mc, int numSliderPacks):
    ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::SliderPack, numSliderPacks)
{}
```

## Modules That Directly Inherit SliderPackProcessor (C++)

Only two modules directly inherit from the C++ `SliderPackProcessor` class:

### 1. ArrayModulator
**File:** `HISE/hi_core/hi_modules/modulators/mods/ArrayModulator.h` line 41

```cpp
class ArrayModulator : public VoiceStartModulator,
                       public SliderPackProcessor
```

- Voice start modulator with 128-element slider pack (one value per MIDI note)
- Passes `SliderPackProcessor(mc, 1)` -- single slider pack
- Calls `referenceShared(ExternalData::DataType::SliderPack, 0)` for shared data support

### 2. BaseHarmonicFilter (and its subclasses)
**File:** `HISE/hi_core/hi_modules/effects/fx/HarmonicFilter.h` line 267

```cpp
class BaseHarmonicFilter: public SliderPackProcessor
```

- Uses 3 slider packs (A, B, Mix): `SliderPackProcessor(mc, 3)`
- Base class for harmonic filter effects

## Important: ExternalDataHolder is the Actual Interface

The scripting wrapper `ScriptSliderPackProcessor` does NOT require the target module to inherit from the C++ `SliderPackProcessor`. The factory method (`Synth.getSliderPackProcessor`) iterates `ExternalDataHolder` instances, and the `getSliderPack` method casts to `ProcessorWithExternalData`. This means ANY module that implements `ExternalDataHolder` and has SliderPack data can be wrapped -- including scriptnode processors, hardcoded script processors, and any custom module.

The mismatch between the scripting class name "SliderPackProcessor" and the actual C++ interface requirement (`ExternalDataHolder`) is an important architectural note. The name is historical -- it refers to the concept of "a processor that has slider packs" rather than requiring the specific C++ base class.

## Builder Integration

The `ScriptSliderPackProcessor` is registered as a Builder interface type:

```cpp
addScriptProcessorInterfaceID<ScriptSliderPackProcessor>(s);
```

And in the `getModuleWithInterface` dispatch:
```cpp
RETURN_IF_MATCH(ScriptSliderPackProcessor, snex::ExternalDataHolder);
```

This confirms the interface type check is `ExternalDataHolder`, not `SliderPackProcessor`.

## Return Type: ScriptSliderPackData

The `getSliderPack()` method returns a `ScriptSliderPackData` instance (HiseScript class name: "SliderPackData").

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` line 1309

```cpp
class ScriptSliderPackData : public ScriptComplexDataReferenceBase,
                             public AssignableObject
```

This is a data reference handle that provides methods for manipulating slider values (setValue, getValue, setNumSliders, etc.). It is a separate API class with its own method set.

## Threading and Lifecycle Constraints

1. **onInit-only:** `Synth.getSliderPackProcessor()` requires `objectsCanBeCreated()` -- must be called during initialization
2. **Audio thread prohibited:** `WARN_IF_AUDIO_THREAD` guard on the factory method
3. **WeakReference safety:** The wrapper stores a `WeakReference<Processor>` -- if the module is deleted, subsequent calls will fail gracefully via `checkValidObject()`
4. **No preprocessor guards:** No `USE_BACKEND` or other conditional compilation around this class

## Related Classes

- **TableProcessor** -- parallel pattern for Table data (`ScriptingTableProcessor` wraps modules with Table data)
- **AudioSampleProcessor** -- parallel pattern for audio file data (`ScriptingAudioSampleProcessor`)
- **DisplayBufferSource** -- parallel pattern for display buffers (`ScriptDisplayBufferSource`)
- **SliderPackData** -- the actual data object returned by `getSliderPack()`

All four share the same architecture: a thin `ConstScriptingObject` wrapper around a module reference, with a single method that creates a data reference object.
