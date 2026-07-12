# ScriptModulationMatrix -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- No prerequisite listed for ScriptModulationMatrix
- `enrichment/resources/survey/class_survey_data.json` -- Entry for ScriptModulationMatrix
- `HISE/hi_scripting/scripting/api/ScriptModulationMatrix.h` -- Class declaration
- `HISE/hi_scripting/scripting/api/ScriptModulationMatrix.cpp` -- Full implementation
- `HISE/hi_core/hi_dsp/modules/ModulationMatrixTools.h` -- MatrixIds namespace, enums, Properties
- `HISE/hi_core/hi_dsp/modules/ModulationMatrixTools.cpp` -- Helper implementations
- `HISE/hi_core/hi_modules/synthesisers/synths/GlobalModulatorContainer.h` -- Upstream container class
- `HISE/hi_core/hi_modules/modulators/mods/MatrixModulator.h` -- MatrixModulator processor
- `HISE/hi_scripting/scripting/api/ScriptingApiContent.h` -- MatrixConnectionBase, ScriptSlider::matrixTargetId
- `HISE/hi_dsp_library/node_api/helpers/modulation.h` -- TargetMode enum
- `HISE/hi_core/hi_core/PresetHandler.h` -- UserPresetStateManager interface
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` -- Factory method Engine::createModulationMatrix

## Class Declaration

File: `HISE/hi_scripting/scripting/api/ScriptModulationMatrix.h`, line 221

```cpp
struct ScriptModulationMatrix : public ConstScriptingObject,
                                public ControlledObject,
                                public UserPresetStateManager
```

### Inheritance Chain

1. **ConstScriptingObject** -- Standard scripting API base (provides `reportScriptError`, `getScriptProcessor`, API method registration)
2. **ControlledObject** -- Access to MainController via `getMainController()`
3. **UserPresetStateManager** (extends RestorableObject) -- Integrates into user preset save/load lifecycle:
   - `getUserPresetStateId()` returns `MatrixIds::MatrixData`
   - `resetUserPresetState()` calls `clearAllConnections({})`
   - `exportAsValueTree()` / `restoreFromValueTree()` for serialization

### Factory Method

Created via `Engine.createModulationMatrix(containerId)`:

```cpp
// ScriptingApi.cpp:3505
juce::var ScriptingApi::Engine::createModulationMatrix(String containerId)
{
    return new ScriptingObjects::ScriptModulationMatrix(getScriptProcessor(), containerId);
}
```

The `containerId` must be the processor ID of a `GlobalModulatorContainer` in the module tree.

## Constructor Analysis

```cpp
ScriptModulationMatrix(ProcessorWithScriptingContent* p, const String& cid) :
    ConstScriptingObject(p, 0),  // 0 constants
    ControlledObject(p->getMainController_()),
    connectionCallback(p, nullptr, var(), 3),
    editCallback(p, nullptr, var(), 1),
    um(getMainController()->getControlUndoManager()),
    sourceSelectionCallback(getScriptProcessor(), this, var(), 1),
    dragCallback(getScriptProcessor(), this, {}, 3) 
```

Key constructor actions:
1. Looks up `GlobalModulatorContainer` by name from `cid` parameter
2. Reports script error if container not found
3. Registers with `UserPresetHandler` via `addStateManager(this)`
4. Sets up `connectionListener` on the container's matrixData ValueTree
5. Fills source/target lists from `MatrixIds::Helpers`

### No addConstant() calls

The constructor passes `0` to `ConstScriptingObject` (no constants). No `addConstant()` calls exist.

### API Method Registration (Wrapper struct)

All methods use plain `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` -- NO typed wrappers in the Wrapper struct.

However, in the constructor body, some methods use `ADD_TYPED_API_METHOD_N`:

```cpp
ADD_TYPED_API_METHOD_1(setConnectionCallback, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_2(setEditCallback, VarTypeChecker::ComplexType, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_1(setSourceSelectionCallback, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_1(setDragCallback, VarTypeChecker::Function);
```

There are also `ADD_CALLBACK_DIAGNOSTIC` registrations:
```cpp
ADD_CALLBACK_DIAGNOSTIC(connectionCallback, setConnectionCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(editCallback, setEditCallback, 1);
ADD_CALLBACK_DIAGNOSTIC(sourceSelectionCallback, setSourceSelectionCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(dragCallback, setDragCallback, 0);
```

All other methods use plain `ADD_API_METHOD_N`.

## Private Members

```cpp
StringArray sourceList;          // filled by MatrixIds::Helpers::fillModSourceList
StringArray allTargets;          // filled by fillModTargetList(..., All)
StringArray parameterTargets;    // filled by fillModTargetList(..., Parameters)
StringArray modulatorTargets;    // filled by fillModTargetList(..., Modulators)

UndoManager* um = nullptr;      // from getMainController()->getControlUndoManager()

WeakCallbackHolder connectionCallback;       // 3 args: sourceId, targetId, wasAdded
WeakCallbackHolder editCallback;             // 1 arg (actually 2 in call: idx, targetId)
WeakCallbackHolder sourceSelectionCallback;  // 1 arg: sourceName
WeakCallbackHolder dragCallback;             // 3 args: sourceId, targetId, actionString

valuetree::ChildListener connectionListener; // watches container matrixData for child add/remove
WeakReference<GlobalModulatorContainer> container;

std::map<String, QueryObject> queryFunctions; // cached modulation display query functions
```

### QueryObject inner struct
```cpp
struct QueryObject
{
    ReferenceCountedObject* slider;
    WeakReference<Processor> p;
    ModulationDisplayValue::QueryFunction::Ptr qf;
};
```

## Upstream Infrastructure: GlobalModulatorContainer

The entire modulation matrix system is backed by a `GlobalModulatorContainer` -- a special `ModulatorSynth` that:
- Hosts global modulators in its GainModulation chain
- Stores all connection data in a `ValueTree matrixData` (type = `MatrixData`)
- Provides broadcasters for:
  - `editCallbackHandler` (LambdaBroadcaster<int, String>)
  - `currentMatrixSourceBroadcaster` (LambdaBroadcaster<int>)
  - `dragBroadcaster` (LambdaBroadcaster<int, String, DragAction>)
- Holds `MatrixIds::Helpers::Properties matrixProperties`
- Manages exclusive source mode via `setExlusiveMatrixSource()`
- Has `customEditCallbacks` StringArray for edit menu items

### Source Discovery

Sources are the child processors of the GlobalModulatorContainer's GainModulation chain:

```cpp
void Helpers::fillModSourceList(const MainController* mc, StringArray& items)
{
    if(auto container = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(...))
    {
        auto mc = container->getChildProcessor(ModulatorSynth::InternalChains::GainModulation);
        for(int i = 0; i < mc->getNumChildProcessors(); i++)
            items.add(mc->getChildProcessor(i)->getId());
    }
}
```

Source IDs are processor IDs (names) of the modulators inside the container.

### Target Discovery

Targets come from two sources:

1. **MatrixModulator processors** -- Iterated via `Processor::Iterator<MatrixModulator>`. Their target ID is `getMatrixTargetId()` (custom ID or processor ID).

2. **ScriptSlider components** with `matrixTargetId` property -- Scanned from the scripting content's component tree. Any component with a non-empty `matrixTargetId` property becomes a target.

The `TargetType` enum distinguishes:
- `All` -- both types
- `Modulators` -- only MatrixModulator processors
- `Parameters` -- only UI component targets

## MatrixIds Namespace (Connection Data Model)

### Connection ValueTree Schema

Each connection is a `ValueTree` of type `Connection` with properties:

| Property | Type | Description |
|----------|------|-------------|
| TargetId | String | Target identifier |
| SourceIndex | int | Index into source list (-1 = disconnected) |
| Intensity | double | Modulation depth, range [-1.0, 1.0] |
| Mode | int | TargetMode enum cast to int |
| Inverted | bool | Whether modulation signal is inverted |
| AuxIndex | int | Secondary source index (-1 = none) |
| AuxIntensity | double | Secondary source depth |

All connections are children of a root `MatrixData` ValueTree.

### Watchable Properties

The set of properties that trigger reactive updates:
```cpp
static Array<Identifier> getWatchableIds() {
    return { SourceIndex, TargetId, Mode, Inverted, Intensity, AuxIndex, AuxIntensity };
}
```

These are the valid `propertyId` values for `getConnectionProperty()` / `setConnectionProperty()`.

### MatrixModulation Properties (Global Config)

The `Properties` struct holds global configuration with three sections:

#### SelectableSources
Boolean flag enabling exclusive source selection mode.

#### DefaultInitValues
Per-target map of default connection values:
```cpp
struct DefaultInitValue
{
    float intensity = 0.0f;
    scriptnode::modulation::TargetMode defaultMode = scriptnode::modulation::TargetMode::Raw;
    bool isNormalised = true;
};
```

JSON schema for DefaultInitValues:
```json
{
  "DefaultInitValues": {
    "targetId": {
      "Intensity": 0.5,
      "IsNormalized": true,
      "Mode": "Scale"  // or "Unipolar" or "Bipolar"
    }
  }
}
```

Mode names (string to enum mapping):
- `"Scale"` -> TargetMode::Gain (0)
- `"Unipolar"` -> TargetMode::Unipolar (1)  
- `"Bipolar"` -> TargetMode::Bipolar (2)

If no init value is configured for a target, defaults are inferred from the target type:
- MatrixModulator in GainMode/GlobalMode: intensity=1.0, mode=Gain
- MatrixModulator in PitchMode/PanMode/OffsetMode/CombinedMode: intensity=0.0, mode=Bipolar
- UI parameter targets: intensity=0.5, mode=Unipolar

#### RangeProperties
Per-target map of range configurations. Can be a preset name string or a full JSON object:

```json
{
  "RangeProperties": {
    "targetId": "FilterFreq",
    "anotherTarget": {
      "InputRange": { "MinValue": 0.0, "MaxValue": 1.0 },
      "OutputRange": { "MinValue": 0.0, "MaxValue": 1.0 },
      "mode": "NormalizedPercentage",
      "UseMidPositionAsZero": false
    }
  }
}
```

### Range Presets

```
NormalizedPercentage  -- [0,1] -> [0,1], converter "NormalizedPercentage"
Gain0dB              -- [-100,0] skewed at -6dB -> [0,1], converter "Decibel"
Gain6dB              -- [-100,6] skewed at 0dB -> [0,2], converter "Decibel"
Pitch1Octave         -- [-12,12] step 1 -> [0,1] bipolar, converter "Semitones"
Pitch2Octaves        -- [-24,24] step 1 -> [-0.5,1.5] bipolar, converter "Semitones"
Pitch1Semitone       -- [-1,1] step 0.01 -> narrow range, converter "Semitones"
PitchOctaveStep      -- [-4,4] step 1 -> [-1.5,2.5] bipolar
PitchSemitoneStep    -- [-12,12] step 1 -> [0,1] stepped bipolar, converter "Semitones"
FilterFreq           -- [20,20000] -> [0,1], converter "Frequency"
FilterFreqLog        -- [20,20000] skewed at 2kHz -> [0,1] skewed, converter "Frequency"
Stereo               -- [-100,100] -> [0,1], converter "Pan", bipolar
```

## TargetMode Enum (from scriptnode modulation.h)

```cpp
enum class TargetMode
{
    Gain,       // Applies HISE intensity-scale modulation (scale mode)
    Unipolar,   // Adds modulation to base value
    Bipolar,    // Adds/subtracts modulation bipolarly
    Pitch,      // Allows values > 1.0
    Raw,        // No processing
    Aux         // Only applies intensity to signal
};
```

In the matrix context, only Gain (0), Unipolar (1), and Bipolar (2) are used for `Mode` property.
The string names used in the Properties JSON are "Scale", "Unipolar", "Bipolar".

## MatrixModulator Processor

A special `EnvelopeModulator` that serves as a modulation target in the module tree:

- `SpecialParameters::Value` -- the base value parameter
- `SpecialParameters::SmoothingTime` -- smoothing for value changes
- Has `getMatrixTargetId()` returning custom ID or processor ID
- Connected to `GlobalModulatorContainer` for signal routing

When a ScriptSlider is connected to a MatrixModulator's Value parameter, `getTargetId()` returns the modulator's ID directly.

## Two Target Mechanism Paths

### Path 1: MatrixModulator targets (modulatorTargets)
- Target is a MatrixModulator processor in the module tree
- Connected via `MultiMatrixModulatorConnection` on the ScriptSlider
- The modulation signal flows through the processor's modulation chain
- Uses `AttributeListener` pattern for real-time updates

### Path 2: Cable-based parameter targets (parameterTargets)
- Target is a ScriptSlider with `matrixTargetId` property set
- Connected via `MatrixCableConnection` on the ScriptSlider
- Uses `GlobalRoutingManager::Cable` for signal transport
- Modulation is applied by recalculating the slider's output value via `calculateNewModValue()`
- The cable connection registers as a `CableTargetBase`

Both paths share the same `MatrixData` ValueTree for connection storage.

## Threading Model

### callSuspended Pattern

Mutating operations use `callSuspended()` which kills voices before executing:

```cpp
void callSuspended(const std::function<void(ScriptModulationMatrix&)>& f)
{
    // Uses KillStateHandler to suspend audio, then executes on ScriptingThread
    // Exception: if already on SampleLoadingThread, executes synchronously
    getMainController()->getKillStateHandler().killVoicesAndCall(p, pf, tt);
}
```

Methods using callSuspended:
- `connect()` -- adding/removing connections
- `fromBase64()` -- restoring state
- `clearAllConnections()` -- removing connections

### UndoManager Integration

Connection property changes go through `getControlUndoManager()`:
- `setConnectionProperty()` uses `c.setProperty(id, value, getControlUndoManager())`
- `restoreFromValueTree()` uses `um` for child add/remove
- Connection additions/removals in `MatrixIds::Helpers` use the undo manager

## Callback Signatures

### connectionCallback (3 args)
```
function(sourceId, targetId, wasAdded)
// sourceId: String (source name from sourceList)
// targetId: String (target ID from connection ValueTree)
// wasAdded: bool (true = connection added, false = removed)
```

Fired by the `connectionListener` whenever a child is added/removed from the matrixData.

### editCallback (2 args despite editCallback init with 1)
```
function(menuIndex, targetId)
// menuIndex: int (index into the menuItems array)
// targetId: String (the target being edited)
```

Note: The WeakCallbackHolder is initialized with argCount=1 but `call(args, 2)` is used. The edit callback is connected to `container->editCallbackHandler` which is a `LambdaBroadcaster<int, String>`.

### sourceSelectionCallback (1 arg)
```
function(sourceName)
// sourceName: String (the name of the newly selected source)
```

Connected to `container->currentMatrixSourceBroadcaster`.

### dragCallback (3 args)
```
function(sourceId, targetId, action)
// sourceId: String (source name, or empty string if si==-1)
// targetId: String
// action: String -- one of: "DragEnd", "DragStart", "Drop", "Hover", "DisabledHover"
```

The action strings are mapped from `GlobalModulatorContainer::DragAction` enum:
```cpp
std::array<var, (int)GlobalModulatorContainer::DragAction::numDragActions> s({
    "DragEnd",     // 0
    "DragStart",   // 1
    "Drop",        // 2
    "Hover",       // 3
    "DisabledHover" // 4
});
```

## Serialization (User Preset Integration)

### UserPresetStateManager Implementation

- `getUserPresetStateId()` returns `MatrixIds::MatrixData` -- the identifier used in the preset ValueTree
- `resetUserPresetState()` calls `clearAllConnections({})` -- removes all connections when preset has no matrix data
- `exportAsValueTree()` returns a copy of the container's matrixData
- `restoreFromValueTree()` replaces all children of the matrixData tree (with undo)

The class registers/unregisters with `UserPresetHandler` in constructor/destructor.

### Base64 Serialization

`toBase64()` and `fromBase64()` use `zstd::ZDefaultCompressor` for ValueTree compression:
- `toBase64()`: compress matrixData ValueTree -> MemoryBlock -> Base64 string
- `fromBase64()`: Base64 -> MemoryBlock -> decompress -> ValueTree -> callSuspended restore

## Display Data

### getModulationDisplayData(targetId)

Returns a JSON object with modulation visualization data for a target. Uses a caching mechanism via `queryFunctions` map.

Two lookup paths:
1. MatrixModulator targets: gets `ModulationDisplayValue::QueryFunction` from the modulator's `getModulationQueryFunction(Value)`
2. Cable-based targets: gets query function from the script processor's `getModulationQueryFunction(componentIndex)`

The `getModulationDataFromQueryFunction()` helper:
- Gets the slider's normalized value and range
- Applies midpoint skew if configured
- Calls `qf->getDisplayValue()` to get a `ModulationDisplayValue`
- Stores result to JSON via `mv.storeToJSON(obj.get())`

### ModulationDisplayValue Properties (inferred from storeToJSON)
The returned JSON object contains modulation display state including normalized values, scaled values, modulation ranges, and active state.

## MatrixConnectionBase (ScriptSlider Integration)

Abstract base on ScriptSlider for receiving matrix modulation:

```cpp
struct MatrixConnectionBase: public ControlledObject
{
    virtual SimpleRingBuffer::Ptr getDisplayBuffer(int index) = 0;
    virtual IntensityTextConverter::ConstructData createIntensityConverter(int sourceIndex) = 0;
    
    WeakReference<ScriptSlider> parent;
    WeakReference<GlobalModulatorContainer> gc;
    ValueTree matrixData;
    String targetId;
};
```

Two concrete implementations:
- `MultiMatrixModulatorConnection` -- for MatrixModulator targets
- `MatrixCableConnection` -- for cable/parameter targets (extensive implementation with Target/AuxTarget subclasses, ring buffers, and real-time modulation calculation)

## Cable-Based Modulation Flow (MatrixCableConnection)

The cable path is more complex:

1. Each connection creates a `Target` object that:
   - Registers as a `CableTargetBase` on a `GlobalRoutingManager::Cable`
   - Receives `sendValue()` calls from the cable
   - Tracks `TargetMode` (Scale/Add), intensity, inversion
   - Optionally has an `AuxTarget` for secondary modulation source

2. Targets are split into `scaleTargets` and `addTargets` lists

3. `calculateNewModValue()` applies all modulation:
   - Starts with normalized slider value
   - Scale targets: multiply `(1-intensity) + intensity * modValue`
   - Add targets: add `intensity * modValue` (with bipolar option)
   - Clamps to [0,1], converts back to slider range
   - Calls `controlCallback()` on the parent script processor

## Error Handling

- Constructor: `reportScriptError(cid + " is not a global modulation container")` if container not found
- `setCurrentlySelectedSource()`: `reportScriptError("Selectable sources are disabled")` if `matrixProperties.selectableSources` is false
- `setMatrixModulationProperties()`: `reportScriptError(iv.first + " init value has no Mode property defined")` if init value has non-zero intensity but no valid mode

## No Preprocessor Guards

No `#if USE_BACKEND` or other preprocessor guards -- the class is available in all build targets.
