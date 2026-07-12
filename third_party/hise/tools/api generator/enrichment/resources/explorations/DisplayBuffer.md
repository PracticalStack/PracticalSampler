# DisplayBuffer -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- no prerequisite listed for DisplayBuffer
- `enrichment/resources/survey/class_survey_data.json` -- DisplayBuffer entry (createdBy: Engine, creates: Buffer, seeAlso: Table, SliderPackData, AudioFile, Buffer, ScriptPanel)
- `enrichment/base/DisplayBuffer.json` -- 8 API methods
- No base class exploration found (no ComplexDataScriptComponent_base.md or similar)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 1206

```cpp
class ScriptRingBuffer : public ScriptComplexDataReferenceBase
{
public:
    ScriptRingBuffer(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* other=nullptr);
    Identifier getObjectName() const override { return Identifier("DisplayBuffer"); }
    // ... 8 API methods ...
private:
    SimpleRingBuffer* getRingBuffer() { return static_cast<SimpleRingBuffer*>(complexObject.get()); }
    const SimpleRingBuffer* getRingBuffer() const { return static_cast<SimpleRingBuffer*>(complexObject.get()); }
    struct Wrapper;
};
```

**Key observations:**
- C++ class name is `ScriptRingBuffer`, but the scripting API name is `DisplayBuffer` (via `getObjectName()`)
- Inherits from `ScriptComplexDataReferenceBase` which provides:
  - `ConstScriptingObject` base (standard scripting API object)
  - `ComplexDataUIUpdaterBase::EventListener` for UI update events
  - `onComplexDataEvent()` -- dispatches DisplayIndex events to `displayCallback` and content changes to `contentCallback`
  - `setCallbackInternal()` and `linkToInternal()` base methods
  - `complexObject` -- `WeakReference<ComplexDataUIBase>` to the underlying data
  - `displayCallback` / `contentCallback` -- `WeakCallbackHolder` for event notifications
  - `index` -- const int index into the holder's data array
  - `holder` -- `WeakReference<ExternalDataHolder>` to the owning processor
- The underlying data object is `SimpleRingBuffer` (cast from `complexObject`)

## Base Class: ScriptComplexDataReferenceBase

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 1076

```cpp
class ScriptComplexDataReferenceBase: public ConstScriptingObject,
                                      public ComplexDataUIUpdaterBase::EventListener
```

Shared by four scripting API complex data handles:
- `ScriptAudioFile` -> AudioFile (wraps `MultiChannelAudioBuffer`)
- `ScriptRingBuffer` -> DisplayBuffer (wraps `SimpleRingBuffer`)
- `ScriptTableData` -> Table (wraps `Table`)
- `ScriptSliderPackData` -> SliderPackData (wraps `SliderPackData`)

Provides:
- `getDataType()` -- returns the `snex::ExternalData::DataType` enum
- `objectDeleted()` / `objectExists()` -- checks `complexObject` weak ref
- `createPopupComponent()` -- for IDE popup editors
- `setPosition()` -- sets display position
- `getCurrentDisplayIndexBase()` -- reads current display index
- `getIndex()` / `getHolder()` -- accessors

**Note:** DisplayBuffer does NOT expose `setDisplayCallback`, `setContentCallback`, or `linkTo` at the scripting API level. The base class has `setCallbackInternal` and `linkToInternal` but ScriptRingBuffer does not add API methods for them (unlike ScriptAudioFile which exposes `setDisplayCallback`, `setContentCallback`, and `linkTo`).

## Constructor -- Method Registration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 1828

```cpp
ScriptingObjects::ScriptRingBuffer::ScriptRingBuffer(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* other):
    ScriptComplexDataReferenceBase(pwsc, index, snex::ExternalData::DataType::DisplayBuffer, other)
{
    ADD_API_METHOD_0(getReadBuffer);
    ADD_API_METHOD_3(createPath);
    ADD_API_METHOD_2(getResizedBuffer);
    ADD_API_METHOD_1(setRingBufferProperties);
    ADD_API_METHOD_1(copyReadBuffer);
    ADD_API_METHOD_1(setActive);
    ADD_API_METHOD_0(toBase64);
    ADD_API_METHOD_2(fromBase64);
}
```

All methods use plain `ADD_API_METHOD_N` -- no typed variants (`ADD_TYPED_API_METHOD_N`). No constants added via `addConstant()`.

## Wrapper Struct (API Method Wrappers)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 1816

```cpp
struct ScriptingObjects::ScriptRingBuffer::Wrapper
{
    API_METHOD_WRAPPER_0(ScriptRingBuffer, getReadBuffer);
    API_METHOD_WRAPPER_3(ScriptRingBuffer, createPath);
    API_METHOD_WRAPPER_2(ScriptRingBuffer, getResizedBuffer);
    API_VOID_METHOD_WRAPPER_1(ScriptRingBuffer, setActive);
    API_VOID_METHOD_WRAPPER_1(ScriptRingBuffer, setRingBufferProperties);
    API_VOID_METHOD_WRAPPER_1(ScriptRingBuffer, copyReadBuffer);
    API_METHOD_WRAPPER_0(ScriptRingBuffer, toBase64);
    API_VOID_METHOD_WRAPPER_2(ScriptRingBuffer, fromBase64);
};
```

No typed method wrappers.

## Factory / obtainedVia

DisplayBuffer instances are created through two paths:

### Path 1: Engine.createAndRegisterRingBuffer(index)
**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 3483
```cpp
ScriptingObjects::ScriptRingBuffer* ScriptingApi::Engine::createAndRegisterRingBuffer(int index)
{
    return new ScriptingObjects::ScriptRingBuffer(getScriptProcessor(), index);
}
```
Creates a standalone DisplayBuffer registered to the script processor's own external data.

### Path 2: Engine.getComplexData() with DataType::DisplayBuffer
**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 2551
```cpp
case ExternalData::DataType::DisplayBuffer: 
    return var(new ScriptingObjects::ScriptRingBuffer(sp, index, typed));
```
Retrieves a DisplayBuffer from any `ProcessorWithExternalData` by module ID and index.

### Path 3: DisplayBufferSource.getDisplayBuffer(index)
**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 7309
```cpp
var ScriptingObjects::ScriptDisplayBufferSource::getDisplayBuffer(int index)
{
    if (objectExists())
    {
        auto numObjects = source->getNumDataObjects(ExternalData::DataType::DisplayBuffer);
        if (isPositiveAndBelow(index, numObjects))
            return var(new ScriptingObjects::ScriptRingBuffer(getScriptProcessor(), index, 
                dynamic_cast<ProcessorWithExternalData*>(source.get())));
        reportScriptError("Can't find buffer at index " + String(index));
    }
    RETURN_IF_NO_THROW({});
}
```
The `DisplayBufferSource` is obtained via `Synth.getDisplayBufferSource(moduleName)`.

## Underlying Data: SimpleRingBuffer

**File:** `HISE/hi_tools/hi_standalone_components/RingBuffer.h`, line 50

```cpp
struct SimpleRingBuffer: public ComplexDataUIBase,
                         public ComplexDataUIUpdaterBase::EventListener
```

### Core Properties
- `internalBuffer` -- `AudioSampleBuffer` (JUCE multi-channel float buffer) that stores the ring buffer data
- `writeIndex`, `readIndex` -- atomic write index, regular read index for ring buffer mechanics
- `active` -- bool, can be disabled to stop writing
- `sr` -- double, sample rate of the data
- `maxLength` -- double, optional max length limit
- `properties` -- `PropertyObject::Ptr`, type-specific behavior object (polymorphic)
- `currentWriter` -- `WeakReference<WriterBase>`, the DSP node currently writing to this buffer

### Key methods
- `setRingBufferSize(numChannels, numSamples, acquireLock)` -- resize internal buffer
- `write(value, numSamples)` / `write(data, numChannels, numSamples)` / `write(buffer, startSample, numSamples)` -- DSP thread writes
- `read(buffer)` -- UI thread reads
- `getReadBuffer()` -- returns const ref to the current read buffer
- `setActive(shouldBeActive)` -- enable/disable
- `setProperty(id, value)` / `getProperty(id)` -- delegated to PropertyObject
- `setPropertyObject(newObject)` -- sets the type-specific PropertyObject
- `registerPropertyObject<T>()` -- registers a PropertyObject type by index for lazy creation

### Built-in property identifiers (RingBufferIds namespace)
```cpp
namespace RingBufferIds
{
    DECLARE_ID(BufferLength);
    DECLARE_ID(NumChannels);
    DECLARE_ID(Active);
}
```

## PropertyObject System

**File:** `HISE/hi_tools/hi_standalone_components/RingBuffer.h`, line 80

The `SimpleRingBuffer::PropertyObject` is a polymorphic handler that customizes ring buffer behavior per source type. Each DSP node type that writes to a display buffer registers its own PropertyObject subclass.

### PropertyObject base interface:
- `getClassIndex()` -- returns the PropertyIndex constant
- `validateInt(id, v)` -- sanitizes buffer size/channel count
- `initialiseRingBuffer(b)` -- sets up buffer size/channels on first connect
- `setProperty(id, value)` / `getProperty(id)` -- type-specific property storage
- `transformReadBuffer(b)` -- post-process read data before display (e.g., FFT transform, envelope state copy)
- `createPath(sampleRange, valueRange, targetBounds, startValue)` -- creates a JUCE Path for visualization
- `createComponent()` -- creates the appropriate RingBufferComponentBase for IDE display
- `exportAsBase64()` / `restoreFromBase64(b64)` -- state serialization (optional, for stateful buffers like AHDSR)

### PropertyObject Factory

**File:** `HISE/hi_dsp_library/hi_dsp_library_01.cpp`, line 129

```cpp
SimpleRingBuffer::PropertyObject* SimpleRingBuffer::createPropertyObject(int propertyIndex, WriterBase* b)
{
    CREATE_PROPERTY_OBJECT(OscillatorDisplayProvider::OscillatorDisplayObject);  // 9000
    CREATE_PROPERTY_OBJECT(ModPlotter::ModPlotterPropertyObject);                // 1000
    CREATE_PROPERTY_OBJECT(scriptnode::envelope::pimpl::simple_ar_base::PropertyObject); // 2001
    CREATE_PROPERTY_OBJECT(scriptnode::envelope::pimpl::ahdsr_base::AhdsrRingBufferProperties); // 2002
    CREATE_PROPERTY_OBJECT(flex_ahdsr_base::Properties);                         // 2003
    CREATE_PROPERTY_OBJECT(scriptnode::analyse::Helpers::Oscilloscope);          // 3002
    CREATE_PROPERTY_OBJECT(scriptnode::analyse::Helpers::FFT);                   // 3001
    CREATE_PROPERTY_OBJECT(scriptnode::analyse::Helpers::GonioMeter);            // 3003
    jassertfalse;
    return nullptr;
}
```

### PropertyObject Subclasses Summary

| PropertyIndex | Class | Source Type | Key Properties |
|---------------|-------|------------|----------------|
| 0 (default) | PropertyObject (base) | Generic | BufferLength, NumChannels, Active |
| 1000 | ModPlotterPropertyObject | Modulator plotter | Fixed size, transform function |
| 2001 | simple_ar_base::PropertyObject | Simple AR envelope | Envelope state display |
| 2002 | ahdsr_base::AhdsrRingBufferProperties | AHDSR envelope | 9-sample UI values buffer |
| 2003 | flex_ahdsr_base::Properties | Flex AHDSR | Base64 state, drag UI, createPath |
| 3001 | analyse::Helpers::FFT | FFT analyser | BufferLength, WindowType, Overlap, DecibelRange, UsePeakDecay, UseDecibelScale, YGamma, Decay, UseLogarithmicFreqAxis |
| 3002 | analyse::Helpers::Oscilloscope | Oscilloscope | BufferLength (128-65536), NumChannels (1-2) |
| 3003 | analyse::Helpers::GonioMeter | Goniometer | BufferLength (512-32768), NumChannels (fixed 2) |
| 9000 | OscillatorDisplayObject | Oscillator display | Fixed size, waveform rendering |

## Method Implementations

### getReadBuffer()
Returns a `VariantBuffer` wrapping channel 0 of the internal read buffer. This is a direct pointer reference -- not a copy. The returned Buffer shares memory with the ring buffer.

### getResizedBuffer(numDestSamples, resampleMode)
Creates a new `VariantBuffer` of the target size, resampled from the read buffer:
- If stride < 2.0: simple point sampling
- If stride >= 2.0: finds min/max in each stride window, takes the midpoint
The `resampleMode` parameter is declared but not used in the implementation (both branches run regardless).

### createPath(dstArea, sourceRange, normalisedStartValue)
- Parses `dstArea` and `sourceRange` as rectangles via `ApiHelpers::getRectangleFromVar()`
- `sourceRange` rectangle encodes: x=min value, y=max value, width=start sample, height=end sample
- Acquires a `SimpleReadWriteLock::ScopedReadLock` on the buffer's data lock
- Delegates to `PropertyObject::createPath()` for type-specific path generation
- Returns a `PathObject` (scripting wrapper for JUCE Path)

### setRingBufferProperties(propertyData)
Iterates over a JSON object's key-value pairs and calls `PropertyObject::setProperty()` for each. The available properties depend on the PropertyObject subclass (see table above).

### copyReadBuffer(targetBuffer)
Copies the read buffer data into a preallocated target buffer. Supports two input forms:
1. Single `Buffer` object: copies channel 0, must match size exactly
2. Array of `Buffer` objects: copies per-channel, must match both channel count and sample count
Acquires `getReadBufferLock()` (CriticalSection) for the copy operation.

### setActive(shouldBeActive)
Calls `SimpleRingBuffer::setActive()` to enable/disable writing to the ring buffer. When inactive, the DSP writer skips the ring buffer write.

### toBase64()
Delegates to `PropertyObject::exportAsBase64()`. Returns empty string for PropertyObject types that don't override this (only flex_ahdsr_base::Properties and potentially others implement it).

### fromBase64(b64, useUndoManager)
Delegates to `PropertyObject::restoreFromBase64()`. When `useUndoManager` is true, wraps the operation in an `UndoableAction` (B64Action inner class) that supports undo/redo through `MainController::getControlUndoManager()`.

## Threading / Lifecycle

- All factory methods (`Engine.createAndRegisterRingBuffer`, etc.) require `objectsCanBeCreated()` -- onInit only
- `getReadBuffer()` returns a direct pointer to shared memory -- reading from the audio thread's write buffer
- `copyReadBuffer()` acquires `getReadBufferLock()` (CriticalSection) for thread-safe copy
- `createPath()` acquires `SimpleReadWriteLock::ScopedReadLock` on the data lock
- The ring buffer is written to by DSP nodes on the audio thread via `SimpleRingBuffer::write()`
- The read buffer is a snapshot prepared by `setupReadBuffer()` / `transformReadBuffer()` -- not the live write buffer
- `setActive(false)` safely disables the audio-thread writer

## DisplayBufferSource Helper Class

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 2531

`ScriptDisplayBufferSource` is a separate scripting API class (API name: `DisplayBufferSource`) that acts as a factory for obtaining DisplayBuffer instances from a processor:

```cpp
class ScriptDisplayBufferSource : public ConstScriptingObject
{
    var getDisplayBuffer(int index);
    // ...
    WeakReference<ExternalDataHolder> source;
};
```

Obtained via `Synth.getDisplayBufferSource(moduleName)`. Not part of the DisplayBuffer API itself but is the primary way to access display buffers from specific modules.

## Processors That Provide DisplayBuffers

Processors that implement `ProcessorWithExternalData` (or its subclasses) can expose display buffers:

### ProcessorWithSingleStaticExternalData
Single data type, single instance. `getDisplayBuffer(index)` returns the owned `SimpleRingBuffer`.

### ProcessorWithStaticExternalData
Multiple data types with fixed counts. Constructor takes `numDisplayBuffers` parameter. Holds `ReferenceCountedArray<SimpleRingBuffer> displayBuffers`.

### ProcessorWithDynamicExternalData
Dynamically resizable data collections. Creates display buffers on demand.

### Known modules with display buffers:
- Analyser effects (FFT, Oscilloscope, Goniometer via scriptnode analyse nodes)
- Envelope modulators (AHDSR, flex AHDSR, simple AR)
- Oscillator display
- MatrixModulator
- CurveEq (FFT buffer)
- HardcodedModuleBase (generic display buffers)
- ScriptModulationMatrix

## Preprocessor Guards

No preprocessor guards found on the DisplayBuffer/ScriptRingBuffer class itself. The class is available in all build targets (backend, frontend, DLL).

The PropertyObject factory in `hi_dsp_library_01.cpp` is guarded by `#if HI_EXPORT_DSP_LIBRARY` (line 1 of that file), meaning the factory registration happens in the DSP library build unit.
