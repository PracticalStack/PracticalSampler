# Table -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- prerequisite check (Table is a prerequisite for TableProcessor, no prerequisite for Table itself)
- `enrichment/resources/survey/class_survey_data.json` -- Table entry (domain: complex-data, role: handle, createdBy: Engine/TableProcessor)
- `enrichment/phase1/TableProcessor/Readme.md` -- downstream consumer context
- `HISE/hi_tools/hi_tools/Tables.h` -- core Table class declaration
- `HISE/hi_tools/hi_tools/Tables.cpp` -- core Table implementation
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` -- ScriptTableData + ScriptComplexDataReferenceBase declarations
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` -- ScriptTableData + ScriptComplexDataReferenceBase implementations
- `HISE/hi_scripting/scripting/api/ScriptingApi.h` -- Engine::createAndRegisterTableData declaration
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` -- Engine factory methods, Synth.getComplexData
- `HISE/hi_scripting/scripting/api/ScriptingApiContent.h` -- ScriptTable (UI component) declaration
- `HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp` -- ComplexDataScriptComponent::registerAtParent

---

## Class Declaration and Inheritance

### Scripting API wrapper: ScriptingObjects::ScriptTableData

File: `hi_scripting/scripting/api/ScriptingApiObjects.h:1250-1305`

```cpp
class ScriptTableData : public ScriptComplexDataReferenceBase
{
public:
    ScriptTableData(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* externalHolder=nullptr);
    Identifier getObjectName() const override { return Identifier("Table"); }
    Component* createPopupComponent(const MouseEvent& e, Component *c) override;
    // ... API methods ...
private:
    Table* getTable() { return static_cast<Table*>(complexObject.get()); }
    const Table* getTable() const { return static_cast<const Table*>(complexObject.get()); }
    struct Wrapper;
};
```

**Key points:**
- The scripting object name is `"Table"` (returned by `getObjectName()`)
- Inherits from `ScriptComplexDataReferenceBase` which provides display/content callback infrastructure, linking, and lifecycle management
- `complexObject` is a `WeakReference<ComplexDataUIBase>` from the base class -- cast to `Table*` via static_cast
- The `ExternalDataHolder* externalHolder` parameter defaults to nullptr; when null, the holder defaults to `dynamic_cast<ExternalDataHolder*>(pwsc)` (the script processor itself)

### Base class: ScriptComplexDataReferenceBase

File: `hi_scripting/scripting/api/ScriptingApiObjects.h:1076-1134`

```cpp
class ScriptComplexDataReferenceBase: public ConstScriptingObject,
                                       public ComplexDataUIUpdaterBase::EventListener
{
public:
    snex::ExternalData::DataType getDataType() const { return type; }
    // ... lifecycle methods ...
    void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data);
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

This base class is shared by all complex data scripting handles: ScriptTableData, ScriptSliderPackData, ScriptAudioFile, ScriptRingBuffer.

### Core data class: Table

File: `hi_tools/hi_tools/Tables.h:50-224`

```cpp
class Table: public ComplexDataUIBase
{
public:
    struct Listener : private ComplexDataUIUpdaterBase::EventListener { ... };
    struct ScopedUpdateDelayer { ... };
    enum DataType { Midi = 0, SampleLookupTable };
    struct GraphPoint { float x, y, curve; };
    // ... methods ...
private:
    bool delayUpdates = false;
    float startY = -1.0f, endY = -1.0f;
    Array<GraphPoint> graphPoints;
    mutable hise::SimpleReadWriteLock graphPointLock;
    ValueTextConverter xConverter, yConverter;
};
```

**Key points:**
- `Table` is abstract (`getTableSize()` and `getWritePointer()` are pure virtual)
- Default state: two points at (0,0,0.5) and (1,1,0.5) -- a linear ramp from 0 to 1
- All graph point access is protected by `SimpleReadWriteLock graphPointLock`
- The `DataType` enum (`Midi`/`SampleLookupTable`) is declared but MidiTable is `#if 0`-ed out -- only `SampleLookupTable` subclass exists

### Subclass: SampleLookupTable

File: `hi_tools/hi_tools/Tables.h:268-359`

```cpp
#define SAMPLE_LOOKUP_TABLE_SIZE 512

class SampleLookupTable: public Table
{
public:
    int getTableSize() const override { return SAMPLE_LOOKUP_TABLE_SIZE; }  // 512
    const float *getReadPointer() const override { return data; };
    float getInterpolatedValue(double sampleIndex, NotificationType notifyEditor) const;
    void setLengthInSamples(double newSampleLength);
    float getFirstValue() const;
    float getLastValue() const;
    int getLengthInSamples() const;
protected:
    float *getWritePointer() override;
private:
    double coefficient;
    float data[SAMPLE_LOOKUP_TABLE_SIZE];  // fixed 512 floats
    int sampleLength;
};
```

This is the ONLY active Table subclass. The internal lookup table is always 512 floats. `coefficient` is computed from `sampleLength` to map external sample indices to the internal 512-element array.

---

## Constructor and Method Registration

File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:2098-2113`

```cpp
ScriptingObjects::ScriptTableData::ScriptTableData(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* otherHolder):
    ScriptComplexDataReferenceBase(pwsc, index, snex::ExternalData::DataType::Table, otherHolder)
{
    ADD_API_METHOD_0(reset);
    ADD_API_METHOD_2(addTablePoint);
    ADD_API_METHOD_4(setTablePoint);
    ADD_API_METHOD_1(getTableValueNormalised);
    ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
    ADD_TYPED_API_METHOD_1(setDisplayCallback, VarTypeChecker::Function);
    ADD_CALLBACK_DIAGNOSTIC(displayCallback, setDisplayCallback, 0);
    ADD_TYPED_API_METHOD_1(setContentCallback, VarTypeChecker::Function);
    ADD_CALLBACK_DIAGNOSTIC(contentCallback, setContentCallback, 0);
    ADD_API_METHOD_1(setTablePointsFromArray);
    ADD_API_METHOD_0(getTablePointsAsArray);
    ADD_API_METHOD_1(linkTo);
}
```

**Typed methods:**
- `setDisplayCallback` -- `VarTypeChecker::Function` (param 1)
- `setContentCallback` -- `VarTypeChecker::Function` (param 1)

**All other methods use untyped ADD_API_METHOD_N.**

**No addConstant() calls** -- Table has no script-visible constants.

---

## Factory Methods / obtainedVia

Table objects are created in three ways:

### 1. Engine.createAndRegisterTableData(index)
File: `hi_scripting/scripting/api/ScriptingApi.cpp:3473-3475`
```cpp
hise::ScriptingObjects::ScriptTableData* ScriptingApi::Engine::createAndRegisterTableData(int index)
{
    return new ScriptingObjects::ScriptTableData(getScriptProcessor(), index);
}
```
Creates a standalone Table data object owned by the script processor at the given slot index.

### 2. TableProcessor.getTable(tableIndex)
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:5130-5135`
```cpp
var ScriptingObjects::ScriptingTableProcessor::getTable(int tableIndex)
{
    // ...
    return var(new ScriptTableData(getScriptProcessor(), tableIndex, ed));
}
```
Creates a Table handle referencing the table owned by a processor module (via its ExternalDataHolder).

### 3. Synth.getComplexData(typeString, processorId, index)
File: `hi_scripting/scripting/api/ScriptingApi.cpp:2546-2548`
```cpp
case ExternalData::DataType::Table: return var(new ScriptingObjects::ScriptTableData(sp, index, typed));
```
Generic complex data accessor -- creates a Table handle when typeString maps to Table.

### 4. ScriptTable.registerAtParent(index)
File: `hi_scripting/scripting/api/ScriptingApiContent.cpp:3329`
```cpp
case ExternalData::DataType::Table: return new ScriptingObjects::ScriptTableData(getScriptProcessor(), index);
```
Registers a ScriptTable UI component's internal table data with the parent processor and returns a Table handle.

---

## Base Class Infrastructure: ScriptComplexDataReferenceBase

### Constructor
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:1508-1521`

- Resolves `holder` to either the provided `otherHolder` or `dynamic_cast<ExternalDataHolder*>(c)` (the script processor)
- Gets the `ComplexDataUIBase` object via `holder->getComplexBaseType(getDataType(), index)`
- Registers as event listener on the data object's updater

### Callback System (onComplexDataEvent)
File: `hi_scripting/scripting/api/ScriptingApiObjects.h:1103-1115`

```cpp
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
```

Two event types:
- `DisplayIndex` -- triggers `displayCallback` (set via `setDisplayCallback`). The `data` parameter is the normalized ruler position (float 0-1).
- `ContentChange` -- triggers `contentCallback` (set via `setContentCallback`). The `data` parameter is the point index that changed, or -1 for bulk changes (reset, setGraphPoints, etc.).

### setCallbackInternal
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:1547-1558`

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

Both callbacks receive exactly 1 argument.

### linkToInternal
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:1560-1594`

Links this Table to another Table's data:
- Validates that `other` is a ScriptComplexDataReferenceBase with matching type
- Unregisters from current event listener
- Calls `holder->linkTo(type, *otherHolder, otherIndex, thisIndex)` on the underlying ExternalDataHolder
- Re-resolves `complexObject` and re-registers event listener

---

## Core Table Methods Implementation

### setTablePoint(pointIndex, x, y, curve)
File: `hi_tools/hi_tools/Tables.cpp:113-140`

- All values are sanitized with `jlimit(0.0f, 1.0f, ...)`
- Uses `SimpleReadWriteLock::ScopedReadLock` for access
- Edge points (index 0 and last) have their x value preserved -- only y and curve are modified
- Non-edge points can have x, y, and curve modified
- After modification: calls `fillLookUpTable()` and sends `ContentChange` notification (unless `delayUpdates` is true)

### addTablePoint(x, y)
File: `hi_tools/hi_tools/Tables.cpp:158-170`

- Uses `SimpleReadWriteLock::ScopedWriteLock`
- Default curve value is 0.5f (from the C++ signature: `addTablePoint(float x, float y, float curve=0.5f)`)
- The scripting wrapper only passes x and y; curve defaults to 0.5
- After adding: sends `ContentChange` notification and calls `fillLookUpTable()` (unless `delayUpdates`)

### reset()
File: `hi_tools/hi_tools/Tables.cpp:142-156`

- Uses `SimpleReadWriteLock::ScopedWriteLock`
- Clears all points and adds two default points: (0, 0, 0.5) and (1, 1, 0.5)
- This creates a linear ramp from 0 to 1
- Sends `ContentChange` notification with pointIndex = -1

### getTableValueNormalised(normalisedInput)
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:2145-2153`

```cpp
float ScriptingObjects::ScriptTableData::getTableValueNormalised(double normalisedInput)
{
    if (auto st = dynamic_cast<SampleLookupTable*>(getTable()))
    {
        return st->getInterpolatedValue(normalisedInput, sendNotificationAsync);
    }
    return 0.0f;
}
```

**Critical:** This method performs a `dynamic_cast` to `SampleLookupTable*`. Since `SampleLookupTable` is the only active Table subclass, this always succeeds for valid tables. The method also triggers a display index notification (sendNotificationAsync) which updates the ruler position and fires the displayCallback.

### getInterpolatedValue (SampleLookupTable)
File: `hi_tools/hi_tools/Tables.h:322-345`

- Input `sampleIndex` is multiplied by `SAMPLE_LOOKUP_TABLE_SIZE` (512) and then by `coefficient`
- Linear interpolation between adjacent lookup table entries
- If index exceeds table size, returns last value
- The `sendNotificationAsync` causes `displayCallback` to fire with the normalized input position

### getTablePointsAsArray()
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:2171-2179` / `hi_tools/hi_tools/Tables.cpp:283-302`

- Delegates to `Table::getTablePointsAsVarArray()`
- Returns array of arrays: `[[x0, y0, curve0], [x1, y1, curve1], ...]`
- Uses `SimpleReadWriteLock::ScopedReadLock` for thread safety

### setTablePointsFromArray(pointList)
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:2181-2213`

- Expects array of 3-element sub-arrays: `[[x, y, curve], ...]`
- Validates each sub-array has exactly 3 elements (reports script error otherwise)
- All values are clamped to 0.0-1.0 range
- First point's x is forced to 0.0, last point's x is forced to 1.0
- Requires at least 2 points (reports script error otherwise)
- Calls `Table::setGraphPoints()` which sorts by x, fills lookup table, and sends ContentChange notification

### getCurrentlyDisplayedIndex()
File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:2156-2159`

- Delegates to `getCurrentDisplayIndexBase()` from the base class
- Returns `complexObject->getUpdater().getLastDisplayValue()` -- the last display position sent to the updater
- Range: 0.0 to 1.0

### setDisplayCallback(displayFunction) / setContentCallback(contentFunction)
Both delegate to `setCallbackInternal(isDisplay, f)` in the base class.

### linkTo(otherTable)
Inline method that delegates to `linkToInternal(otherTable)` in the base class. See base class section above for implementation details.

---

## ScopedUpdateDelayer

File: `hi_tools/hi_tools/Tables.h:69-80`, `Tables.cpp:60-71`

```cpp
struct ScopedUpdateDelayer
{
    ScopedUpdateDelayer(Table& t);
    ~ScopedUpdateDelayer();
    Table& table;
    bool prevValue = false;
};
```

Constructor sets `table.delayUpdates = true`. Destructor calls `fillLookUpTable()`, sends ContentChange notification, and restores previous value.

This is used internally when batch-modifying table points to avoid unnecessary recalculations between individual point operations. **Not exposed to scripting API** -- but it explains why the scripting API has no explicit batch update mechanism. Each individual `addTablePoint`/`setTablePoint` call triggers a full lookup table recalculation.

---

## Lookup Table Filling Mechanism

File: `hi_tools/hi_tools/Tables.cpp:442-498`

The `fillLookUpTable()` method:
1. Allocates a temporary `HeapBlock<float>` of `getTableSize()` elements (512 for SampleLookupTable)
2. Sorts graph points by x coordinate
3. Calls `fillExternalLookupTable()` which:
   - Creates a JUCE `Path` from the graph points
   - Uses `PathFlatteningIterator` to walk the path
   - For each of the 512 output positions, finds the path intersection at that x position
   - Stores `1.0f - y` (inverted) as the lookup value
4. Copies the computed values to the actual data array

**Important:** The y values in the lookup table are inverted (`1.0f - y`) compared to the GraphPoint y values. This is because the Path rendering uses screen coordinates (y-down). However, `getInterpolatedValue()` reads from the final lookup table directly, so the inversion is already baked in.

---

## Threading and Lifecycle

- **Thread safety:** All graph point access uses `SimpleReadWriteLock`. Point modification uses `ScopedWriteLock`, reading uses `ScopedReadLock`.
- **No onInit restriction:** Table objects can be used in any callback context -- they are data objects, not module handles.
- **Notification threading:** `setTablePoint` sends `sendNotificationSync`, while `addTablePoint` and `reset` send `sendNotificationAsync`. This means `setTablePoint` triggers synchronous UI updates while `addTablePoint` and `reset` use async notification.
- **fillLookUpTable() cost:** Each call creates a path, iterates it with PathFlatteningIterator, and writes 512 floats. This is moderately expensive and happens on every point modification (unless ScopedUpdateDelayer is active).

---

## createPopupComponent (USE_BACKEND only)

File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:2115-2125`

```cpp
Component* ScriptingObjects::ScriptTableData::createPopupComponent(const MouseEvent& e, Component *c)
{
#if USE_BACKEND
    auto te = dynamic_cast<Component*>(snex::ExternalData::createEditor(getTable()));
    te->setSize(300, 200);
    return te;
#else
    ignoreUnused(e, c);
    return nullptr;
#endif
}
```

Only available in the HISE IDE (USE_BACKEND). Creates a 300x200 table editor popup when the object is inspected in the debugger.

---

## Wrapper Struct (Method Binding)

File: `hi_scripting/scripting/api/ScriptingApiObjects.cpp:2084-2096`

```cpp
struct ScriptingObjects::ScriptTableData::Wrapper
{
    API_VOID_METHOD_WRAPPER_0(ScriptTableData, reset);
    API_VOID_METHOD_WRAPPER_4(ScriptTableData, setTablePoint);
    API_VOID_METHOD_WRAPPER_2(ScriptTableData, addTablePoint);
    API_METHOD_WRAPPER_1(ScriptTableData, getTableValueNormalised);
    API_METHOD_WRAPPER_0(ScriptTableData, getCurrentlyDisplayedIndex);
    API_VOID_METHOD_WRAPPER_1(ScriptTableData, setDisplayCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptTableData, setContentCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptTableData, setTablePointsFromArray);
    API_METHOD_WRAPPER_0(ScriptTableData, getTablePointsAsArray);
    API_VOID_METHOD_WRAPPER_1(ScriptTableData, linkTo);
};
```

All methods use standard untyped wrappers (`API_VOID_METHOD_WRAPPER_N` / `API_METHOD_WRAPPER_N`). The typed enforcement is only in the constructor registrations for the two callback methods.

---

## Relationship to ScriptTable (UI Component)

The `ScriptTable` UI component (`ScriptingApiContent.h:1440-1496`) is a separate class that wraps a Table for visual editing. Key relationships:

- `ScriptTable` inherits from `ComplexDataScriptComponent`, not from `ScriptTableData`
- `ScriptTable` has its own methods like `setTablePopupFunction`, `setSnapValues`, `referToData`, `registerAtParent`, `setMouseHandlingProperties`
- `ScriptTable.registerAtParent(index)` creates and returns a `ScriptTableData` (Table) object
- `ScriptTable.referToData(tableData)` connects the UI to an existing Table data object
- The Table data object is the programmatic API; ScriptTable is the visual component

---

## Relationship to TableProcessor

Per the TableProcessor Readme, `TableProcessor` wraps any `Processor` that implements `ExternalDataHolder` (typically via `LookupTableProcessor`). `TableProcessor.getTable(tableIndex)` creates a `ScriptTableData` handle pointing to the processor's table.

Table is the **data model**; TableProcessor is the **module access handle**. Table can exist independently (via `Engine.createAndRegisterTableData()`) or be obtained from a module via TableProcessor.
