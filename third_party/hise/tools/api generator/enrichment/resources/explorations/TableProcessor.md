# TableProcessor -- C++ Source Exploration

## Resources Consulted

- `enrichment/base/TableProcessor.json` -- 7 API methods (exists, addTablePoint, setTablePoint, reset, exportAsBase64, restoreFromBase64, getTable)
- `enrichment/resources/survey/class_survey_data.json` -- TableProcessor entry (domain: audio, role: handle, createdBy: [Builder, Synth], creates: [Table])
- `enrichment/phase1/Synth/Readme.md` -- prerequisite class analysis (factory method context)
- No base class exploration needed (not a component class)

## Class Declaration

**File:** `hi_scripting/scripting/api/ScriptingApiObjects.h`, line 2554

```cpp
class ScriptingTableProcessor : public ConstScriptingObject
{
public:
    ScriptingTableProcessor(ProcessorWithScriptingContent *p, ExternalDataHolder *tableProcessor);
    ~ScriptingTableProcessor() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("TableProcessor"); }

    Identifier getObjectName() const override { return getClassName(); };
    bool objectDeleted() const override { return tableProcessor.get() == nullptr; }
    bool objectExists() const override { return tableProcessor != nullptr; }

    // API Methods
    bool exists() { return checkValidObject(); };
    void setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve);
    void addTablePoint(int tableIndex, float x, float y);
    void reset(int tableIndex);
    void restoreFromBase64(int tableIndex, const String& state);
    String exportAsBase64(int tableIndex) const;
    var getTable(int tableIndex);

    struct Wrapper;

private:
    WeakReference<Processor> tableProcessor;
};
```

### Inheritance

- `ConstScriptingObject` -- standard HISE scripting object base class
- No additional interfaces (no ControlledObject, no WeakErrorHandler)
- Very simple class -- single `WeakReference<Processor>` member, no complex state

### Key Observations

- Constructor accepts `ExternalDataHolder*` but stores it as `WeakReference<Processor>` via `dynamic_cast<Processor*>(tableProcessor_)`. This means the internal reference is to the underlying Processor, not the ExternalDataHolder interface.
- `objectDeleted()` / `objectExists()` check the WeakReference for null -- standard weak-reference lifecycle pattern.

## Constructor Analysis

**File:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 5032

```cpp
ScriptingObjects::ScriptingTableProcessor::ScriptingTableProcessor(
    ProcessorWithScriptingContent *p, ExternalDataHolder *tableProcessor_) :
    ConstScriptingObject(p, dynamic_cast<Processor*>(tableProcessor_) != nullptr 
        ? dynamic_cast<Processor*>(tableProcessor_)->getNumParameters() : 0),
    tableProcessor(dynamic_cast<Processor*>(tableProcessor_))
{
    if (tableProcessor != nullptr)
    {
        setName(tableProcessor->getId());

        for (int i = 0; i < tableProcessor->getNumParameters(); i++)
        {
            addConstant(tableProcessor->getIdentifierForParameterIndex(i).toString(), var(i));
        }
    }
    else
    {
        setName("Invalid Processor");
    }

    ADD_API_METHOD_3(addTablePoint);
    ADD_API_METHOD_1(reset);
    ADD_API_METHOD_5(setTablePoint);
    ADD_API_METHOD_1(exportAsBase64);
    ADD_API_METHOD_2(restoreFromBase64);
    ADD_API_METHOD_1(getTable);
}
```

### Dynamic Constants

The constructor dynamically registers the underlying processor's parameters as named constants. For each parameter index `i`, it calls `addConstant(parameterName, i)`. This means the available constants depend entirely on which module type the TableProcessor wraps:

- A `VelocityModulator` would expose its parameters as constants
- A `TableEnvelope` would expose Attack, Release, etc.
- A `ModulatorSampler` would expose sampler-specific parameters

These constants allow `setAttribute()`-style access using named constants rather than raw indices, though TableProcessor itself does not expose `setAttribute`/`getAttribute` methods. The constants are inherited from `ConstScriptingObject` infrastructure.

### Method Registration

All methods use plain `ADD_API_METHOD_N` -- no typed variants (`ADD_TYPED_API_METHOD_N`).

### Wrapper Struct

```cpp
struct ScriptingObjects::ScriptingTableProcessor::Wrapper
{
    API_VOID_METHOD_WRAPPER_3(ScriptingTableProcessor, addTablePoint);
    API_VOID_METHOD_WRAPPER_1(ScriptingTableProcessor, reset);
    API_VOID_METHOD_WRAPPER_5(ScriptingTableProcessor, setTablePoint);
    API_METHOD_WRAPPER_1(ScriptingTableProcessor, exportAsBase64);
    API_VOID_METHOD_WRAPPER_2(ScriptingTableProcessor, restoreFromBase64);
    API_METHOD_WRAPPER_1(ScriptingTableProcessor, getTable);
};
```

No typed API method wrappers. All use standard `API_VOID_METHOD_WRAPPER_N` or `API_METHOD_WRAPPER_N`.

## Factory Methods (obtainedVia)

### 1. Synth.getTableProcessor(name)

**File:** `hi_scripting/scripting/api/ScriptingApi.cpp`, line 6117

```cpp
ScriptingObjects::ScriptingTableProcessor *ScriptingApi::Synth::getTableProcessor(const String &name)
{
    WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

    if (getScriptProcessor()->objectsCanBeCreated())
    {
        Processor::Iterator<ExternalDataHolder> it(owner);
        while (auto lut = it.getNextProcessor())
        {
            if (dynamic_cast<Processor*>(lut)->getId() == name)
                return new ScriptTableProcessor(getScriptProcessor(), lut);
        }
        reportScriptError(name + " was not found. ");
        RETURN_IF_NO_THROW(new ScriptTableProcessor(getScriptProcessor(), nullptr));
    }
    else
    {
        reportIllegalCall("getScriptingTableProcessor()", "onInit");
        RETURN_IF_NO_THROW(new ScriptTableProcessor(getScriptProcessor(), nullptr));
    }
}
```

Key details:
- **onInit-only restriction** via `objectsCanBeCreated()` -- matches Synth prerequisite analysis
- **Owner-rooted search** using `Processor::Iterator<ExternalDataHolder>(owner)` -- searches only the parent synth's subtree
- Iterates ALL `ExternalDataHolder` implementations, not just `LookupTableProcessor`. This means it can find any module that implements `ExternalDataHolder::getTable()`, including modules with multiple data types.
- Registered with `CHECK_MODULE(getTableProcessor, ProcessorWithExternalData)` (line 5440)

### 2. Modulator.asTableProcessor()

**File:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 3330

```cpp
var ScriptingObjects::ScriptingModulator::asTableProcessor()
{
    if (checkValidObject())
    {
        auto ltp = dynamic_cast<LookupTableProcessor*>(mod.get());
        if (ltp == nullptr)
            return var(); // don't complain here, handle it on scripting level
        auto t = new ScriptingTableProcessor(getScriptProcessor(), ltp);
        return var(t);
    }
    auto t = new ScriptingObjects::ScriptingTableProcessor(getScriptProcessor(), nullptr);
    return var(t);
}
```

Key details:
- Converts an existing `Modulator` reference to a `TableProcessor` handle
- Uses `dynamic_cast<LookupTableProcessor*>` -- only works for modulators that inherit LookupTableProcessor
- Returns `var()` (undefined) silently if the modulator has no table -- does NOT report an error
- No onInit restriction (uses existing modulator reference)

### 3. Builder.get(id, "TableProcessor")

**File:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp`, lines 10277 and 10415

The Builder's `get()` method uses interface type matching:
```cpp
addScriptProcessorInterfaceID<ScriptingTableProcessor>(s);  // registers "TableProcessor" as interface type
// ...
RETURN_IF_MATCH(ScriptingTableProcessor, snex::ExternalDataHolder);  // matches via ExternalDataHolder
```

This means `Builder.get(id, "TableProcessor")` will match any processor that implements `snex::ExternalDataHolder`.

## ExternalDataHolder Interface Hierarchy

The class relies on the ExternalData system, which is a unified interface for accessing complex data objects (Tables, SliderPacks, AudioFiles, DisplayBuffers, FilterData).

### Hierarchy

```
ExternalDataHolder (snex namespace -- abstract interface)
  -> getTable(int index) -> Table*
  -> getSliderPack(int index) -> SliderPackData*
  -> getAudioFile(int index) -> MultiChannelAudioBuffer*
  -> getDisplayBuffer(int index) -> SimpleRingBuffer*
  -> getFilterData(int index) -> FilterDataObject*
  -> getNumDataObjects(DataType t) -> int

ProcessorWithExternalData : ExternalDataHolder (hi_core)
  ProcessorWithSingleStaticExternalData : ProcessorWithExternalData
    LookupTableProcessor : ProcessorWithSingleStaticExternalData
  ProcessorWithStaticExternalData : ProcessorWithExternalData
  ProcessorWithDynamicExternalData : ProcessorWithExternalData
```

### LookupTableProcessor Subclasses (modules that can be obtained as TableProcessor)

From grep results across `hi_core/hi_modules/`:

| Module | File | Description |
|--------|------|-------------|
| VelocityModulator | modulators/mods/VelocityModulator.h | Maps velocity to modulation via table |
| TableEnvelope | modulators/mods/TableEnvelope.h | Table-based envelope shape |
| RandomModulator | modulators/mods/RandomModulator.h | Randomized modulation with table curve |
| PitchWheelModulator | modulators/mods/PitchWheelModulator.h | Pitch wheel to modulation via table |
| MPEModulator | modulators/mods/MPEModulators.h | MPE data to modulation via table |
| MacroControlModulator | modulators/mods/MacroControlModulator.h | Macro control to modulation via table |
| KeyModulator | modulators/mods/KeyModulator.h | Key number to modulation via table |
| GlobalModulator | modulators/mods/GlobalModulators.h | Global modulation routing with table |
| ControlModulator | modulators/mods/ControlModulator.h | Control signal to modulation via table |
| ShapeFX | effects/fx/ShapeFX.h | Waveshaping effect with table curve |
| ModulatorSampler | hi_sampler/sampler/ModulatorSampler.h | Sample player (table for crossfade etc.) |

Additionally, any `ProcessorWithStaticExternalData` or `ProcessorWithDynamicExternalData` module that has tables can be found by `Synth.getTableProcessor()` since it iterates `ExternalDataHolder`, not just `LookupTableProcessor`.

## Table Class (Underlying Data)

**File:** `hi_tools/hi_tools/Tables.h`, line 50

```cpp
class Table: public ComplexDataUIBase
```

The `Table` class represents an editable lookup table with graph points rendered to a float array. Key features:

- **DataType enum:** `Midi` (128 elements) or `SampleLookupTable` (2048 elements)
- **GraphPoint struct:** `{float x, float y, float curve}` -- all normalized 0.0-1.0 coordinates, curve controls interpolation curvature
- **Key methods used by ScriptingTableProcessor:**
  - `setTablePoint(int pointIndex, float x, float y, float curve)` -- modify existing point
  - `addTablePoint(float x, float y, float curve=0.5f)` -- add new point (default curve 0.5)
  - `reset()` -- reset to default 0..1 line
  - `exportData()` -> String -- base64 export
  - `restoreData(const String&)` -- base64 restore
- **ScopedUpdateDelayer:** batches notifications when making multiple point changes

## Method Implementation Patterns

All methods follow the same pattern:

1. Check `tableProcessor != nullptr`
2. Cast to `ExternalDataHolder*` via `dynamic_cast<ExternalDataHolder*>(tableProcessor.get())`
3. Call `getTable(tableIndex)` to get the `Table*` for the specified index
4. If table is null, call `reportScriptError("No table")`
5. Delegate to the `Table` method

**Exception:** `getTable()` (the ScriptTableData factory) uses a different pattern:
1. Calls `checkValidObject()`
2. Casts to `ProcessorWithExternalData*` (not just `ExternalDataHolder*`)
3. Creates a `new ScriptTableData(getScriptProcessor(), tableIndex, ed)` -- wrapping the table as a script-accessible complex data object

### tableIndex Parameter

All methods accept a `tableIndex` parameter. Most `LookupTableProcessor` modules have exactly one table (index 0), but some modules (like `ProcessorWithStaticExternalData` or `ProcessorWithDynamicExternalData` subclasses) can have multiple tables. The index selects which table to operate on.

There is no bounds checking at the ScriptingTableProcessor level -- it relies on `ExternalDataHolder::getTable(index)` returning `nullptr` for invalid indices, which triggers the "No table" error.

## ScriptTableData (Created by getTable)

**File:** `hi_scripting/scripting/api/ScriptingApiObjects.h`, line 1250

```cpp
class ScriptTableData : public ScriptComplexDataReferenceBase
```

The `getTable()` method creates a `ScriptTableData` object -- this is the scripting API class named `Table` (object name "Table"). It wraps the same underlying `Table` data but provides its own API surface including:
- `setTablePoint`, `addTablePoint`, `reset` (same as TableProcessor but without tableIndex)
- `getTableValueNormalised(double)` -- evaluate the table at a position
- `getCurrentlyDisplayedIndex()` -- current ruler position
- `setDisplayCallback(var)` -- callback for ruler changes

## Threading and Lifecycle

- **Object creation restricted to onInit** via `objectsCanBeCreated()` check in `Synth.getTableProcessor()`
- **No explicit thread safety** in the TableProcessor methods themselves -- they delegate directly to `Table` methods
- **WeakReference** ensures safe access if the underlying processor is deleted -- `objectDeleted()` returns true
- **WARN_IF_AUDIO_THREAD** guard on factory method only, not on individual table manipulation methods

## Preprocessor Guards

None. The TableProcessor class has no conditional compilation guards. It is available in all build targets (backend, frontend, project DLL).

## No Typed API Methods

All methods use plain `ADD_API_METHOD_N` registration. There are zero `ADD_TYPED_API_METHOD_N` registrations.
