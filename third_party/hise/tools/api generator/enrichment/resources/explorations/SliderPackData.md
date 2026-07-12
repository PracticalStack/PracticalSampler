# SliderPackData -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- prerequisites table (row 14: SliderPackData -> ScriptSliderPack)
- `enrichment/resources/survey/class_survey_data.json` -- SliderPackData entry (createdBy, seeAlso, etc.)
- `enrichment/base/SliderPackData.json` -- authoritative method list (18 methods)
- No prerequisite Readme to load (SliderPackData IS the prerequisite for ScriptSliderPack)
- No existing base class exploration for ComplexDataReferenceBase

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:1309-1400`

```cpp
class ScriptSliderPackData : public ScriptComplexDataReferenceBase,
                             public AssignableObject
```

**Exposed scripting name:** `SliderPackData` (via `getObjectName()` returning `Identifier("SliderPackData")`)

### Inheritance Chain

1. **ScriptComplexDataReferenceBase** (ScriptingApiObjects.h:1076)
   - **ConstScriptingObject** -- base for all read/write scripting API objects
   - **ComplexDataUIUpdaterBase::EventListener** -- receives display index and content change events
2. **AssignableObject** (ScriptingBaseObjects.h:612)
   - Provides `operator[]` support: `assign()`, `getAssignedValue()`, `getCachedIndex()`
   - Enables `spData[index] = value` and `var x = spData[index]` syntax in HiseScript

### Key Base Class: ScriptComplexDataReferenceBase

This is the shared base for all four complex data scripting handles: SliderPackData, Table, AudioFile, DisplayBuffer.

**Members:**
- `WeakReference<ComplexDataUIBase> complexObject` -- weak ref to the underlying data object
- `WeakCallbackHolder displayCallback` -- callback fired on display index changes
- `WeakCallbackHolder contentCallback` -- callback fired on content changes
- `const snex::ExternalData::DataType type` -- enum identifying data type (SliderPack)
- `WeakReference<ExternalDataHolder> holder` -- the processor that owns the data slots
- `const int index` -- slot index within the holder

**Constructor** (ScriptingApiObjects.cpp:1508):
```cpp
ScriptComplexDataReferenceBase(ProcessorWithScriptingContent* c, int dataIndex, 
    snex::ExternalData::DataType type_, ExternalDataHolder* otherHolder = nullptr)
```
- Gets the ExternalDataHolder from `otherHolder` or falls back to `dynamic_cast<ExternalDataHolder*>(c)`
- Retrieves the complex data object via `holder->getComplexBaseType(getDataType(), index)`
- Registers as event listener on the updater

**Key base methods:**
- `setCallbackInternal(bool isDisplay, var f)` -- sets display or content callback (validates HiseJavascriptEngine::isJavascriptFunction)
- `linkToInternal(var o)` -- links this data slot to another's data, type-checked
- `getCurrentDisplayIndexBase()` -- returns last display value from updater
- `setPosition(double)` -- sends display change message

## Underlying Data Model: SliderPackData (hi_tools)

**File:** `HISE/hi_tools/hi_standalone_components/SliderPack.h:41-181`

```cpp
class SliderPackData: public SafeChangeBroadcaster,
                      public ComplexDataUIBase
```

This is the non-scripting data model that actually stores the float array. The scripting wrapper (`ScriptSliderPackData`) delegates all operations to this class.

### Internal Storage

- `VariantBuffer::Ptr dataBuffer` -- the actual float data, stored as a JUCE `VariantBuffer`
- `HeapBlock<float> preallocatedData` -- optional preallocated memory for resize-without-loss
- `int numPreallocated = 0` -- size of preallocated block (0 = not using preallocation)
- `Range<double> sliderRange` -- min/max range for values
- `double stepSize` -- quantization step size (default: 0.01)
- `float defaultValue` -- default value for new sliders (default: 1.0f)
- `int nextIndexToDisplay` -- currently highlighted slider index (default: -1)
- `bool flashActive` -- whether flash overlay is shown
- `bool showValueOverlay` -- whether value overlay is shown

### Default State
- Default number of sliders: `NumDefaultSliders = 16`
- Default range: [0.0, 1.0]
- Default step size: 0.01
- Default value: 1.0f (new sliders are filled with 1.0, not 0.0)

### Thread Safety
- Uses `SimpleReadWriteLock` via `getDataLock()` (inherited from ComplexDataUIBase)
- Read lock acquired for: `getValue()`, `getNumSliders()`, `getCachedData()`, `writeToFloatArray()`, `setFromFloatArray()`
- Write lock acquired for: `setNumSliders()` (when using preallocated data), `setUsePreallocatedLength()`
- `setValue()` uses read lock (single sample write is safe with read lock since VariantBuffer sample writes are atomic-like)

### Undo Support
- `SliderPackAction` inner struct (UndoableAction) supports both single-value and multi-value undo
- Single-value: stores old/new value for one slider index
- Multi-value: stores old/new arrays for bulk operations
- Undo manager obtained via `getUndoManager(bool useUndoManager)` from ComplexDataUIBase

### Listener System
- `SliderPackData::Listener` (inner class) wraps ComplexDataUIUpdaterBase::EventListener
- Three virtual callbacks:
  - `sliderPackChanged(SliderPackData*, int index)` -- on ContentChange
  - `sliderAmountChanged(SliderPackData*)` -- on ContentRedirected
  - `displayedIndexChanged(SliderPackData*, int newIndex)` -- on DisplayIndex
- Listeners are bridged through `onComplexDataEvent()` which maps EventType to the three virtuals

### Notification Events
The underlying SliderPackData uses ComplexDataUIUpdaterBase with three event types:
- `ContentChange` -- a value changed (carries the slider index as int, or -1 for bulk)
- `ContentRedirected` -- the buffer was swapped/resized (num sliders changed)
- `DisplayIndex` -- the display position changed (carries float index)

## Constructor Registration

**File:** `ScriptingApiObjects.cpp:2236-2259`

All methods use `ADD_API_METHOD_N` (untyped) except:
- `setDisplayCallback` -- `ADD_TYPED_API_METHOD_1(setDisplayCallback, VarTypeChecker::Function)`
- `setContentCallback` -- `ADD_TYPED_API_METHOD_1(setContentCallback, VarTypeChecker::Function)`
- `fromBase64` -- `ADD_TYPED_API_METHOD_1(fromBase64, VarTypeChecker::String)`

### Callback Diagnostics
```cpp
ADD_CALLBACK_DIAGNOSTIC(displayCallback, setDisplayCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(contentCallback, setContentCallback, 0);
```
Both callbacks accept 1 argument (the `WeakCallbackHolder` is initialized with arg count 1).

### getStepSize -- Missing Registration

**Important finding:** `getStepSize()` is declared in the header (line 1322), has a Wrapper method, and is implemented (line 2261), but it is **NOT** registered via `ADD_API_METHOD_0(getStepSize)` in the constructor. It appears in the base JSON, so it exists in the API reference, but it may not actually be callable from script. The Wrapper struct does NOT contain an entry for getStepSize either -- confirmed by checking lines 2215-2234. This method is likely a dead/unreachable API method.

## Wrapper Struct

**File:** `ScriptingApiObjects.cpp:2215-2234`

All methods use plain `API_METHOD_WRAPPER_N` or `API_VOID_METHOD_WRAPPER_N` macros. No typed wrappers at the Wrapper struct level -- typing is done at the `ADD_TYPED_API_METHOD` level in the constructor.

```
API_VOID_METHOD_WRAPPER_2(setValue)
API_VOID_METHOD_WRAPPER_1(setNumSliders)
API_METHOD_WRAPPER_1(getValue)
API_METHOD_WRAPPER_0(getNumSliders)
API_VOID_METHOD_WRAPPER_3(setRange)
API_METHOD_WRAPPER_0(getCurrentlyDisplayedIndex)
API_VOID_METHOD_WRAPPER_1(setDisplayCallback)
API_VOID_METHOD_WRAPPER_1(setContentCallback)
API_VOID_METHOD_WRAPPER_1(setUsePreallocatedLength)
API_VOID_METHOD_WRAPPER_1(setAllValuesWithUndo)
API_VOID_METHOD_WRAPPER_1(setAllValues)
API_VOID_METHOD_WRAPPER_2(setValueWithUndo)
API_METHOD_WRAPPER_0(getDataAsBuffer)
API_VOID_METHOD_WRAPPER_1(linkTo)
API_VOID_METHOD_WRAPPER_1(setAssignIsUndoable)
API_METHOD_WRAPPER_0(toBase64)
API_VOID_METHOD_WRAPPER_1(fromBase64)
```

Note: No wrapper for `getStepSize` -- confirms it's unregistered.

## Factory / obtainedVia

Three creation paths:

1. **Engine.createAndRegisterSliderPackData(index)** -- ScriptingApi.cpp:3468-3471
   ```cpp
   return new ScriptingObjects::ScriptSliderPackData(getScriptProcessor(), index);
   ```
   Registered as `ADD_API_METHOD_1(createAndRegisterSliderPackData)` in the Engine constructor.

2. **ScriptSliderPack.getReferenceToComplexData()** -- ScriptingApiContent.cpp:3330
   ```cpp
   case ExternalData::DataType::SliderPack: 
       return new ScriptingObjects::ScriptSliderPackData(getScriptProcessor(), index);
   ```
   Created when a ScriptSliderPack UI component's data reference is requested.

3. **Via ExternalDataHolder** -- ScriptingApiObjects.cpp:5363
   ```cpp
   return var(new ScriptSliderPackData(getScriptProcessor(), sliderPackIndex, ed));
   ```
   When getting data from an external data holder (e.g., a scriptnode node).

4. **Via ScriptingApi.cpp:2549** -- from typed external data holder
   ```cpp
   case ExternalData::DataType::SliderPack: 
       return var(new ScriptingObjects::ScriptSliderPackData(sp, index, typed));
   ```

## AssignableObject Implementation

Enables `[]` operator syntax for read/write access to slider values.

```cpp
void assign(const int index, var newValue) override
{
    if(assignIsUndoable)
        setValueWithUndo(index, (float)newValue);
    else
        setValue(index, (float)newValue);
}

var getAssignedValue(int index) const override
{
    return getValue(index);
}

int getCachedIndex(const var& indexExpression) const override
{
    return (int)indexExpression;
}
```

The `assignIsUndoable` flag (default `false`) controls whether `[]` assignments go through the undo system. This is toggled by `setAssignIsUndoable(bool)`.

## Method Implementation Details (Infrastructure-Level)

### setAllValues / setAllValuesWithUndo

Both accept `var value` which can be:
- A **single scalar** -- sets ALL sliders to that value
- An **Array** -- sets sliders from array values
- A **Buffer** -- sets sliders from buffer samples

Implementation (ScriptingApiObjects.cpp:2336-2385):
```cpp
auto isMultiValue = value.isBuffer() || value.isArray();
int maxIndex = value.isBuffer() ? (value.getBuffer()->size) 
             : (value.isArray() ? value.size() : d->getNumSliders());
```

When multi-value, iterates to `maxIndex` and reads `value[i]`. When scalar, uses `(float)value` for all indices. The resulting float array is passed to `SliderPackData::setFromFloatArray()` with `sendNotificationAsync`.

The `WithUndo` variant passes `true` as the useUndoManager parameter.

### linkTo

Delegated to `ScriptComplexDataReferenceBase::linkToInternal()` (ScriptingApiObjects.cpp:1560-1594):
- Validates the argument is a ScriptComplexDataReferenceBase
- Validates type matches (must both be SliderPack)
- Calls `pdst->linkTo(type, *psrc, other->index, index)` on the ExternalDataHolder
- Re-registers as event listener on the new complex object

This effectively makes two SliderPackData handles point to the same underlying data.

### getDataAsBuffer

Inline implementation (header line 1344-1350):
```cpp
var getDataAsBuffer()
{
    if(auto d = getSliderPackData())
        return d->getDataArray();
    return var();
}
```

`getDataArray()` returns `var(dataBuffer.get())` -- the VariantBuffer is returned BY REFERENCE, not copied. Modifications to the returned Buffer directly modify the underlying slider pack data.

### Preallocated Length

`setUsePreallocatedLength(int length)` configures a fixed-size memory block:
- When set (length > 0): allocates HeapBlock, copies current data into it, makes VariantBuffer reference this memory
- When cleared (length = 0): creates a new owned VariantBuffer, copies data back, frees HeapBlock
- When `setNumSliders()` is called with preallocation active: resizes by adjusting the referToData length (up to numPreallocated), preserving existing values
- Without preallocation: `setNumSliders()` creates a new VariantBuffer, copies existing values up to the smaller size, fills new slots with `defaultValue` (1.0f)

## No Constants

The constructor has no `addConstant()` calls. This class has no scripting constants.

## No Preprocessor Guards

No `#if USE_BACKEND` or other conditional compilation in any of the ScriptSliderPackData code. All functionality is available in all build targets.

## Event Callback Data Flow

The base class `onComplexDataEvent` dispatches events to the script callbacks:

```cpp
void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
    if (t == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
    {
        if(displayCallback)
            displayCallback.call1(data);  // data = float display index
    }
    else
    {
        if(contentCallback)
            contentCallback.call1(data);  // data = int slider index (-1 for bulk)
    }
}
```

- **displayCallback** receives a float (the display index position)
- **contentCallback** receives an int (the changed slider index, or -1 for bulk changes like setAllValues/setFromFloatArray)

## Relationship to ScriptSliderPack (UI Component)

SliderPackData is the prerequisite for ScriptSliderPack (class_survey row 14). The UI component wraps and displays the data model. The creation path for getting a SliderPackData from a UI component is via `getReferenceToComplexData()`.

Key distinction: SliderPackData is the data model that can exist independently of any UI. ScriptSliderPack is the visual component. You can have SliderPackData without ScriptSliderPack (e.g., for scriptnode data, step sequencer logic), but ScriptSliderPack always has an associated SliderPackData.
