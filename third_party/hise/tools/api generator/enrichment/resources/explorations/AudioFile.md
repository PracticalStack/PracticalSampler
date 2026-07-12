# AudioFile (ScriptAudioFile) -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisites table (no prerequisite for AudioFile)
- `enrichment/resources/survey/class_survey_data.json` -- AudioFile entry (domain: complex-data, role: handle)
- `enrichment/base/AudioFile.json` -- 14 API methods
- No prerequisite class required (AudioFile does not appear in the "Then Enrich" column with dependencies)
- No existing base class exploration needed (AudioFile's base `ScriptComplexDataReferenceBase` is explored inline below)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 1136

```cpp
class ScriptAudioFile : public ScriptComplexDataReferenceBase
{
public:
    ScriptAudioFile(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* otherHolder = nullptr);
    Identifier getObjectName() const override { return Identifier("AudioFile"); }

    // ... 14 API methods ...

private:
    MultiChannelAudioBuffer* getBuffer() { return static_cast<MultiChannelAudioBuffer*>(complexObject.get()); }
    const MultiChannelAudioBuffer* getBuffer() const { return static_cast<const MultiChannelAudioBuffer*>(complexObject.get()); }
    struct Wrapper;
};
```

**Script-facing name:** `AudioFile`
**Internal C++ class:** `ScriptingObjects::ScriptAudioFile`
**Inheritance:** `ScriptAudioFile -> ScriptComplexDataReferenceBase -> ConstScriptingObject, ComplexDataUIUpdaterBase::EventListener`

## Base Class: ScriptComplexDataReferenceBase

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, lines 1076-1134

This is the shared base for all complex data reference types:
- `ScriptAudioFile` (AudioFile) -- wraps `MultiChannelAudioBuffer`
- `ScriptRingBuffer` (DisplayBuffer) -- wraps `SimpleRingBuffer`
- `ScriptTableData` (Table) -- wraps `Table`
- `ScriptSliderPackData` (SliderPackData) -- wraps `SliderPackData`

### Key Members

```cpp
class ScriptComplexDataReferenceBase: public ConstScriptingObject,
                                      public ComplexDataUIUpdaterBase::EventListener
{
public:
    snex::ExternalData::DataType getDataType() const { return type; }
    bool objectDeleted() const override { return complexObject == nullptr; }
    bool objectExists() const override { return complexObject != nullptr; }

    ScriptComplexDataReferenceBase(ProcessorWithScriptingContent* c, int dataIndex,
                                   snex::ExternalData::DataType type, ExternalDataHolder* otherHolder=nullptr);
    virtual ~ScriptComplexDataReferenceBase();

    void setPosition(double newPosition);
    float getCurrentDisplayIndexBase() const;
    int getIndex() const { return index; }
    ExternalDataHolder* getHolder() { return holder; }

    void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
    {
        if (t == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
        {
            if(displayCallback)
                displayCallback.call1(data);
        }
        else
        {
            if(contentCallback)
                contentCallback.call1(data);
        }
    }

protected:
    void setCallbackInternal(bool isDisplay, var f);
    void linkToInternal(var o);
    WeakReference<ComplexDataUIBase> complexObject;
    WeakCallbackHolder displayCallback;
    WeakCallbackHolder contentCallback;

private:
    const snex::ExternalData::DataType type;
    WeakReference<ExternalDataHolder> holder;
    const int index;
};
```

### Base Class Constructor Implementation

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1508-1521

```cpp
ScriptComplexDataReferenceBase::ScriptComplexDataReferenceBase(
    ProcessorWithScriptingContent* c, int dataIndex,
    snex::ExternalData::DataType type_, ExternalDataHolder* otherHolder) :
    ConstScriptingObject(c, 0),
    index(dataIndex),
    type(type_),
    holder(otherHolder != nullptr ? otherHolder : dynamic_cast<ExternalDataHolder*>(c)),
    displayCallback(c, this, var(), 1),
    contentCallback(c, this, var(), 1)
{
    if (holder != nullptr)
    {
        if((complexObject = holder->getComplexBaseType(getDataType(), index)))
            complexObject->getUpdater().addEventListener(this);
    }
}
```

Key pattern: If `otherHolder` is null, falls back to `dynamic_cast<ExternalDataHolder*>(c)` -- meaning the script processor itself is the data holder. When `otherHolder` is provided, the AudioFile references external data from another processor.

### Callback Infrastructure (Base Class)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1547-1557

```cpp
void ScriptComplexDataReferenceBase::setCallbackInternal(bool isDisplay, var f)
{
    if (HiseJavascriptEngine::isJavascriptFunction(f))
    {
        auto& cb = isDisplay ? displayCallback : contentCallback;
        cb = WeakCallbackHolder(getScriptProcessor(), this, f, 1);
        cb.incRefCount();
        cb.setThisObject(this);
        cb.addAsSource(this, "onComplexDataEvent");
    }
}
```

Both callbacks receive 1 argument. The `onComplexDataEvent` dispatch routes:
- `EventType::DisplayIndex` -> `displayCallback.call1(data)` (data = display position value)
- Any other event type -> `contentCallback.call1(data)` (data = -1 for content changes)

### linkTo Implementation (Base Class)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1560-1594

```cpp
void ScriptComplexDataReferenceBase::linkToInternal(var o)
{
    auto other = dynamic_cast<ScriptComplexDataReferenceBase*>(o.getObject());
    if(other == nullptr)
    {
        reportScriptError("Not a data object");
        return;
    }
    if(other->type != type)
    {
        reportScriptError("Type mismatch");
        return;
    }

    if(auto pdst = holder.get())
    {
        if(auto psrc = other->holder.get())
        {
            if(auto ex = psrc->getComplexBaseType(type, other->index))
            {
                complexObject->getUpdater().removeEventListener(this);
                pdst->linkTo(type, *psrc, other->index, index);
                complexObject = holder->getComplexBaseType(type, index);
                complexObject->getUpdater().addEventListener(this);
            }
        }
    }
}
```

Validates type match, then delegates to the holder's `linkTo()`. Re-registers event listener on the new complex object. Error conditions: null object, type mismatch (e.g. trying to link AudioFile to Table).

## Constructor and Method Registration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1615-1635

```cpp
ScriptAudioFile::ScriptAudioFile(ProcessorWithScriptingContent* pwsc, int index_,
                                  ExternalDataHolder* otherHolder) :
    ScriptComplexDataReferenceBase(pwsc, index_, snex::ExternalData::DataType::AudioFile, otherHolder)
{
    ADD_API_METHOD_0(getRange);
    ADD_API_METHOD_0(getTotalLengthInSamples);
    ADD_API_METHOD_2(setRange);
    ADD_API_METHOD_1(getLoopRange);
    ADD_API_METHOD_1(loadFile);
    ADD_API_METHOD_0(getContent);
    ADD_API_METHOD_0(update);
    ADD_API_METHOD_0(getNumSamples);
    ADD_API_METHOD_0(getSampleRate);
    ADD_API_METHOD_0(getCurrentlyLoadedFile);
    ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
    ADD_TYPED_API_METHOD_1(setDisplayCallback, VarTypeChecker::Function);
    ADD_CALLBACK_DIAGNOSTIC(displayCallback, setDisplayCallback, 0);
    ADD_TYPED_API_METHOD_1(setContentCallback, VarTypeChecker::Function);
    ADD_CALLBACK_DIAGNOSTIC(contentCallback, setContentCallback, 0);
    ADD_API_METHOD_1(linkTo);
    ADD_API_METHOD_3(loadBuffer);
}
```

### Forced-Type Methods

| Method | Macro | Param 1 |
|--------|-------|---------|
| `setDisplayCallback` | `ADD_TYPED_API_METHOD_1` | `VarTypeChecker::Function` |
| `setContentCallback` | `ADD_TYPED_API_METHOD_1` | `VarTypeChecker::Function` |

### Callback Diagnostics

Both `setDisplayCallback` and `setContentCallback` use `ADD_CALLBACK_DIAGNOSTIC` with 0 arguments specified -- this enables parse-time warnings about callback usage.

### No Constants

The constructor has no `addConstant()` calls. AudioFile has zero script-visible constants.

## Wrapper Struct (API Method Wrappers)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1596-1613

```cpp
struct ScriptingObjects::ScriptAudioFile::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, loadFile);
    API_METHOD_WRAPPER_0(ScriptAudioFile, getContent);
    API_VOID_METHOD_WRAPPER_0(ScriptAudioFile, update);
    API_METHOD_WRAPPER_0(ScriptAudioFile, getRange);
    API_METHOD_WRAPPER_0(ScriptAudioFile, getTotalLengthInSamples);
    API_VOID_METHOD_WRAPPER_2(ScriptAudioFile, setRange);
    API_METHOD_WRAPPER_1(ScriptAudioFile, getLoopRange);
    API_VOID_METHOD_WRAPPER_3(ScriptAudioFile, loadBuffer);
    API_METHOD_WRAPPER_0(ScriptAudioFile, getNumSamples);
    API_METHOD_WRAPPER_0(ScriptAudioFile, getSampleRate);
    API_METHOD_WRAPPER_0(ScriptAudioFile, getCurrentlyLoadedFile);
    API_METHOD_WRAPPER_0(ScriptAudioFile, getCurrentlyDisplayedIndex);
    API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, setDisplayCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, setContentCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, linkTo);
};
```

All methods use plain `API_METHOD_WRAPPER` / `API_VOID_METHOD_WRAPPER` -- no typed wrappers (typing is done at registration, not wrapping).

## Factory Methods / obtainedVia

AudioFile instances are created via three paths:

### 1. Engine.createAndRegisterAudioFile(index)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, lines 3478-3481

```cpp
ScriptingObjects::ScriptAudioFile* ScriptingApi::Engine::createAndRegisterAudioFile(int index)
{
    return new ScriptingObjects::ScriptAudioFile(getScriptProcessor(), index);
}
```

Creates an AudioFile referencing the script processor's own data slot at the given index.

### 2. AudioSampleProcessor.getAudioFile(slotIndex)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 4997-5007

```cpp
var ScriptingObjects::ScriptingAudioSampleProcessor::getAudioFile(int slotIndex)
{
    if (checkValidObject())
    {
        if (auto ed = dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get()))
            return var(new ScriptAudioFile(getScriptProcessor(), slotIndex, ed));
    }
    reportScriptError("Not a valid object");
    RETURN_IF_NO_THROW(var());
}
```

Creates an AudioFile referencing an external processor's data slot. The `otherHolder` parameter points to the target processor's ExternalDataHolder.

### 3. ComplexDataScriptComponent (ScriptAudioWaveform internal)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp`, lines 3327-3334

```cpp
switch (type)
{
case ExternalData::DataType::Table: return new ScriptTableData(...);
case ExternalData::DataType::SliderPack: return new ScriptSliderPackData(...);
case ExternalData::DataType::AudioFile: return new ScriptAudioFile(getScriptProcessor(), index);
default: jassertfalse; return var();
}
```

Created internally by ScriptAudioWaveform when connecting to a data source.

## Underlying Data: MultiChannelAudioBuffer

**File:** `HISE/hi_tools/hi_standalone_components/SampleDisplayComponent.h`, lines 499-748

`MultiChannelAudioBuffer` is the core data object that `ScriptAudioFile` wraps. It extends `ComplexDataUIBase` and manages:

### Key Data Members

```cpp
Range<int> bufferRange;       // Current sub-range within the original buffer
Range<int> loopRange;         // Loop points
String referenceString;       // File reference (pool reference or path)
AudioSampleBuffer originalBuffer;  // Full loaded audio data
AudioSampleBuffer currentData;     // Sub-range extracted data
DataProvider::Ptr provider;        // Handles file loading
double sampleRate = 0.0;           // Sample rate of loaded audio
```

### Data Architecture

The buffer maintains two layers:
1. **originalBuffer** -- the full audio file as loaded
2. **currentData** -- a sub-range copy created by `setRange()`

`getBuffer()` returns `currentData`, not `originalBuffer`. This is significant: `getNumSamples()` returns the size of the current range, not the total file length. Use `getTotalLengthInSamples()` for the original file length.

### SampleReference

```cpp
struct SampleReference : public ReferenceCountedObject
{
    AudioSampleBuffer buffer;
    Result r;
    String reference = {};
    Range<int> loopRange = {};
    double sampleRate = 0.0;
};
```

This is the intermediate format returned by `DataProvider::loadFile()`. It carries the loaded audio, sample rate, loop points, and the resolved reference string.

### DataProvider

```cpp
struct DataProvider: public ReferenceCountedObject
{
    virtual SampleReference::Ptr loadFile(const String& referenceString) = 0;
    virtual File parseFileReference(const String& b64) const = 0;
    virtual File getRootDirectory();
};
```

Implementations:
- `PooledAudioFileDataProvider` (`HISE/hi_core/hi_core/ExternalFilePool.h:753`) -- standard pool-based loading from HISE audio file pool
- `XYZSampleMapProvider::FileBasedDataProvider` -- file-based loading for XYZ multi-sample maps
- `XYZSampleMapProvider::MonolithDataProvider` -- monolith-based loading for XYZ data

### XYZ Provider System

MultiChannelAudioBuffer supports an advanced "XYZ" mode for multi-sample data (key/velocity/round-robin mapped samples). This is used primarily by WavetableSynth and similar processors.

```cpp
struct XYZItem
{
    Range<int> veloRange;
    Range<int> keyRange;
    double root;
    int rrGroup;
    SampleReference::Ptr data;
};
```

When `fromBase64String()` detects an XYZ provider ID in the reference string, it routes through the XYZ provider system instead of the standard single-file path. The `XYZProviderFactory` uses a shared resource pattern to register and create providers by ID.

### Listener Interface

```cpp
struct Listener: private ComplexDataUIUpdaterBase::EventListener
{
    virtual void bufferWasLoaded() = 0;    // Synchronous, holds data write lock
    virtual void bufferWasModified() = 0;  // Synchronous, no lock
    virtual void sampleIndexChanged(int newSampleIndex); // Asynchronous
};
```

C++ modules (AudioLooper, ConvolutionEffect, WavetableSynth, NoiseGrainPlayer) implement this listener to respond to audio data changes. The script-side callbacks (`setContentCallback`, `setDisplayCallback`) map to the same updater events.

## fromBase64String -- File Loading Path

**File:** `HISE/hi_tools/hi_standalone_components/SampleDisplayComponent.cpp`, lines 2688-2770

`loadFile()` calls `buffer->fromBase64String(filePath)`. Despite the name, this is the primary file loading mechanism. The "filePath" is actually a reference string, not necessarily base64.

The loading path branches:

1. **Empty string** -- clears XYZ items if XYZ provider active, sends content redirect
2. **XYZ reference** (detected via `XYZProviderFactory::parseID()`) -- routes to XYZ provider's `parse()` method, which populates `xyzItems` list
3. **Standard reference** -- uses the DataProvider to load the file:
   - DataProvider resolves the reference string to audio data
   - Sets `originalBuffer`, `bufferRange`, `sampleRate`, `loopRange`
   - Creates new data buffer for the full range
   - Sends content redirect message via updater

The reference string format depends on the DataProvider. For `PooledAudioFileDataProvider`, this is a HISE pool reference like `{PROJECT_FOLDER}audiofile.wav`.

## loadBuffer -- Programmatic Buffer Loading

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1709-1744

```cpp
void ScriptAudioFile::loadBuffer(var bufferData, double sampleRate, var loopRange)
{
    Range<int> lr;
    if(loopRange.isArray() && loopRange.size() == 2)
    {
        lr = { (int)loopRange[0], (int)loopRange[1] };
    }

    if(auto buffer = getBuffer())
    {
        if(bufferData.isArray())
        {
            // Array of Buffer objects -- multi-channel
            float* ptrs[NUM_MAX_CHANNELS];
            int numChannels = bufferData.size();
            int numSamples = 0;
            for(int i = 0; i < bufferData.size(); i++)
            {
                if (auto b = bufferData[i].getBuffer())
                {
                    numSamples = b->buffer.getNumSamples();
                    ptrs[i] = b->buffer.getWritePointer(0);
                }
            }
            AudioSampleBuffer ab(ptrs, numChannels, numSamples);
            buffer->loadBuffer(ab, sampleRate, lr);
        }
        else if (auto b = bufferData.getBuffer())
        {
            // Single Buffer object -- mono
            buffer->loadBuffer(b->buffer, sampleRate, lr);
        }
    }
}
```

Accepts either:
- A single `Buffer` object (mono)
- An `Array` of `Buffer` objects (multi-channel, one Buffer per channel)

The `loopRange` parameter is optional (can be empty array or undefined) -- if it's a 2-element array, it sets loop points.

## getContent -- Channel Data Access

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1752-1763

```cpp
var ScriptAudioFile::getContent()
{
    Array<var> channels;
    if (auto buffer = getBuffer())
    {
        for (int i = 0; i < buffer->getBuffer().getNumChannels(); i++)
            channels.add(buffer->getChannelBuffer(i, false));
    }
    return channels;
}
```

Returns an Array of Buffer objects, one per channel. The `false` parameter to `getChannelBuffer` means it returns the current range data (not full content). This aligns with getNumSamples() returning the range size.

## setRange / getRange

**setRange** (lines 1643-1668):
```cpp
void ScriptAudioFile::setRange(int min, int max)
{
    if (auto buffer = getBuffer())
    {
        int numChannels = buffer->getBuffer().getNumChannels();
        if (numChannels == 0) { clear(); return; }
        min = jmax(0, min);
        max = jmin(buffer->getBuffer().getNumSamples(), max);
        int size = max - min;
        if (size == 0) { clear(); return; }
        buffer->setRange({ min, max });
    }
}
```

Clamps to valid range. Zero-size range clears the buffer. Delegates to `MultiChannelAudioBuffer::setRange()`.

**getRange** (lines 1670-1682):
Returns `[start, end]` array from `buffer->getCurrentRange()`.

## getTotalLengthInSamples vs getNumSamples

- `getTotalLengthInSamples()` -- returns `buffer->getTotalRange().getEnd()` (original file length)
- `getNumSamples()` -- returns `buffer->getBuffer().getNumSamples()` (current range/sub-buffer length)

This distinction is important: after `setRange()`, `getNumSamples()` returns the range size, while `getTotalLengthInSamples()` returns the full original file size.

## update Method

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 1770-1776

```cpp
void ScriptAudioFile::update()
{
    if (auto buffer = getBuffer())
    {
        buffer->getUpdater().sendContentChangeMessage(sendNotificationAsync, -1);
    }
}
```

Sends an asynchronous content change message with data value -1. This triggers all registered content listeners (both C++ Listeners and script contentCallbacks).

## getCurrentlyLoadedFile / loadFile

`getCurrentlyLoadedFile()` returns `buffer->toBase64String()` -- the reference string (not actual base64 of audio data).

`loadFile(filePath)` calls `buffer->fromBase64String(filePath)` -- loads from a reference string (e.g. `{PROJECT_FOLDER}myfile.wav`).

## ExternalData::DataType Enum

**File:** `HISE/hi_dsp_library/snex_basics/snex_ExternalData.h`, lines 369-378

```cpp
enum class DataType: int
{
    Table,              // look up table with 512 float numbers
    SliderPack,         // resizable array of float numbers
    AudioFile,          // multichannel audio file with metadata (loop points, samplerate)
    FilterCoefficients, // filter coefficients for display
    DisplayBuffer,      // FIFO buffer for analysis/visualisation
    numDataTypes,
    ConstantLookUp
};
```

AudioFile is the third data type (index 2).

## ExternalDataHolder Interface

**File:** `HISE/hi_dsp_library/snex_basics/snex_ExternalData.h`, line 595

The `ExternalDataHolder` interface provides:
- `getComplexBaseType(DataType, index)` -- retrieves the underlying ComplexDataUIBase
- `getAudioFile(index)` -- returns `MultiChannelAudioBuffer*`
- `linkTo(type, source, srcIndex, dstIndex)` -- links data slots between holders

Processors that hold audio file slots implement this interface:
- `ProcessorWithExternalData` (script processors) -- `HISE/hi_core/hi_dsp/ProcessorInterfaces.h`
- `HardcodedModuleBase` -- `HISE/hi_core/hi_modules/hardcoded/HardcodedModuleBase.h`

## Processors Using MultiChannelAudioBuffer::Listener

From grep results, these C++ modules listen to audio buffer changes:

| Module | File |
|--------|------|
| AudioLooper | `hi_core/hi_modules/synthesisers/synths/AudioLooper.h` |
| WavetableSynth | `hi_core/hi_modules/synthesisers/synths/WavetableSynth.h` |
| ConvolutionEffect | `hi_core/hi_modules/effects/fx/Convolution.h` |
| NoiseGrainPlayer | `hi_core/hi_modules/effects/fx/NoiseGrainPlayer.h` |

These are the processors whose audio file slots can be accessed through `AudioSampleProcessor.getAudioFile()`.

## Threading Considerations

- `fromBase64String()` acquires `SimpleReadWriteLock::ScopedWriteLock` on the data lock before modifying buffers
- `setRange()` on the MultiChannelAudioBuffer level also acquires write lock
- Listener callbacks (`bufferWasLoaded`) are called synchronously while holding the write lock
- `sampleIndexChanged` is called asynchronously
- Script callbacks go through `WeakCallbackHolder` which handles message thread dispatch
- The `update()` method uses `sendNotificationAsync` -- always asynchronous

## Preprocessor Guards

No preprocessor guards specific to AudioFile. The class is available in all build configurations (backend, frontend, project DLL).

## Related UI Component: ScriptAudioWaveform

`ScriptAudioWaveform` is the visual counterpart -- it displays the audio data that `AudioFile` manages. The AudioFile is the data handle; ScriptAudioWaveform is the UI component. They connect via the ComplexDataScriptComponent infrastructure using the same `ExternalDataHolder` and slot index.
