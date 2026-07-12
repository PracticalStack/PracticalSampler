# RoutingMatrix -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` (RoutingMatrix entry)
- `enrichment/phase1/Synth/Readme.md` (prerequisite -- module tree system)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` (class declaration, lines 2161-2227)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` (implementation, lines 3959-4190)
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` (Synth::getRoutingMatrix factory, line 6278)
- `HISE/hi_core/hi_dsp/Routing.h` (RoutableProcessor + MatrixData, full file)
- `HISE/hi_core/hi_dsp/Routing.cpp` (MatrixData implementation, full file)
- `HISE/hi_scripting/scripting/api/ScriptBroadcaster.h` (RoutingMatrixListener, line 589)
- `HISE/hi_scripting/scripting/api/ScriptBroadcaster.cpp` (RoutingMatrixListener impl, lines 1763-1903)
- `HISE/hi_tools/Macros.h` (NUM_MAX_CHANNELS = 16)
- `HISE/hi_core/LibConfig.h` (HISE_NUM_PLUGIN_CHANNELS = 2)

---

## Class Declaration

File: `hi_scripting/scripting/api/ScriptingApiObjects.h`, lines 2161-2227

```cpp
class ScriptRoutingMatrix : public ConstScriptingObject
{
public:
    ScriptRoutingMatrix(ProcessorWithScriptingContent *p, Processor *processor);
    ~ScriptRoutingMatrix() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("RoutingMatrix"); }
    Identifier getObjectName() const override { return getClassName(); }
    bool objectDeleted() const override { return rp.get() == nullptr; }
    bool objectExists() const override { return rp != nullptr; }

    // Debug interface
    String getDebugName() const override;
    String getDebugDataType() const override;
    String getDebugValue() const override;
    void doubleClickCallback(const MouseEvent &, Component*) override {};

    // API Methods
    void setNumChannels(int numSourceChannels);
    int getNumSourceChannels();
    int getNumDestinationChannels();
    bool addConnection(int sourceIndex, int destinationIndex);
    bool addSendConnection(int sourceIndex, int destinationIndex);
    bool removeSendConnection(int sourceIndex, int destinationIndex);
    bool removeConnection(int sourceIndex, int destinationIndex);
    void clear();
    float getSourceGainValue(int channelIndex);
    var getSourceChannelsForDestination(var destinationIndex) const;
    var getDestinationChannelForSource(var sourceIndex) const;

    // Internal accessor
    RoutableProcessor* getRoutableProcessor() { return dynamic_cast<RoutableProcessor*>(rp.get()); }

    struct Wrapper;

private:
    WeakReference<Processor> rp;
};
```

Key observations:
- Inherits from `ConstScriptingObject` (no `AssignableObject`)
- Holds a `WeakReference<Processor>` to the target processor
- All method calls go through `dynamic_cast<RoutableProcessor*>(rp.get())` to access the underlying MatrixData
- Constructor passes `2` to `ConstScriptingObject` for the number of constants (NumInputs, NumOutputs)
- No typed API methods -- all use plain `ADD_API_METHOD_N` / `API_METHOD_WRAPPER_N`

---

## Constructor and Constants

File: `ScriptingApiObjects.cpp`, lines 3974-4001

```cpp
ScriptingObjects::ScriptRoutingMatrix::ScriptRoutingMatrix(ProcessorWithScriptingContent *p, Processor *processor):
    ConstScriptingObject(p, 2),
    rp(processor)
{
    ADD_API_METHOD_2(addConnection);
    ADD_API_METHOD_2(removeConnection);
    ADD_API_METHOD_2(addSendConnection);
    ADD_API_METHOD_2(removeSendConnection);
    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_1(getSourceGainValue);
    ADD_API_METHOD_1(setNumChannels);
    ADD_API_METHOD_0(getNumSourceChannels);
    ADD_API_METHOD_0(getNumDestinationChannels);
    ADD_API_METHOD_1(getSourceChannelsForDestination);
    ADD_API_METHOD_1(getDestinationChannelForSource);

    if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
    {
        addConstant("NumInputs", r->getMatrix().getNumSourceChannels());
        addConstant("NumOutputs", r->getMatrix().getNumDestinationChannels());
    }
    else
    {
        jassertfalse;
        addConstant("NumInputs", -1);
        addConstant("NumOutputs", -1);
    }
}
```

### Constants Table

| Name | Type | Description |
|------|------|-------------|
| NumInputs | int | Number of source (input) channels at construction time |
| NumOutputs | int | Number of destination (output) channels at construction time |

These are snapshot values captured when the RoutingMatrix object is created. They reflect the channel count at that point; if `setNumChannels` is later called, the constants do NOT update (they are `addConstant`, not dynamic properties).

---

## Wrapper Struct (Method Registration)

File: `ScriptingApiObjects.cpp`, lines 3959-3972

All methods use plain `API_METHOD_WRAPPER_N` or `API_VOID_METHOD_WRAPPER_N` -- no typed variants (`ADD_TYPED_API_METHOD_N`). This means all parameter types must be inferred from method signatures.

---

## Factory Methods / obtainedVia

### Via Synth.getRoutingMatrix(processorId)

File: `ScriptingApi.cpp`, lines 6278-6291

```cpp
ScriptRoutingMatrix* ScriptingApi::Synth::getRoutingMatrix(const String& processorId)
{
    auto p = ProcessorHelpers::getFirstProcessorWithName(
        getScriptProcessor()->getMainController_()->getMainSynthChain(), processorId);

    if (p == nullptr)
        reportScriptError(processorId + " was not found");

    if (auto rt = dynamic_cast<RoutableProcessor*>(p))
        return new ScriptingObjects::ScriptRoutingMatrix(getScriptProcessor(), p);
    else
        reportScriptError(processorId + " does not have a routing matrix");

    RETURN_IF_NO_THROW(new ScriptingObjects::ScriptRoutingMatrix(getScriptProcessor(), nullptr));
}
```

Key details:
- Uses **global-rooted search** from `getMainSynthChain()` (as documented in Synth prerequisite)
- Not restricted to `onInit` (no `objectsCanBeCreated()` check)
- Validates that the found processor implements `RoutableProcessor` interface
- Reports script error if processor not found or doesn't have a routing matrix

### Via ChildSynth.getRoutingMatrix()

File: `ScriptingApiObjects.cpp`, lines 4534-4538

```cpp
var ScriptingObjects::ScriptingSynth::getRoutingMatrix()
{
    auto r = new ScriptRoutingMatrix(getScriptProcessor(), synth.get());
    return var(r);
}
```

- Creates a RoutingMatrix from the ChildSynth's internal `synth` WeakReference
- No explicit RoutableProcessor check here (ModulatorSynth always inherits RoutableProcessor)

---

## Underlying Infrastructure: RoutableProcessor

File: `hi_core/hi_dsp/Routing.h`, full file (213 lines)

### RoutableProcessor Class

```cpp
class RoutableProcessor
{
public:
    enum class Presets
    {
        AllChannels = 10000,
        FirstStereo,
        SecondStereo,
        ThirdStereo,
        AllChannelsToStereo
    };

    static constexpr float SilenceThreshold = -90.f;

    // Pure virtual callbacks for channel count changes
    virtual void numSourceChannelsChanged() = 0;
    virtual void numDestinationChannelsChanged() = 0;
    virtual void connectionChanged();  // default empty implementation

    const MatrixData &getMatrix() const;
    MatrixData &getMatrix();

    // Quick accessor shortcuts (used internally)
    int getLeftSourceChannel() const;
    int getRightSourceChannel() const;
    int getLeftDestinationChannel() const;
    int getRightDestinationChannel() const;

    MatrixData data;  // public member

private:
    int leftSourceChannel;
    int rightSourceChannel;
    int leftTargetChannel;
    int rightTargetChannel;
};
```

### Which Processors Implement RoutableProcessor

Three base classes in HISE inherit from `RoutableProcessor`:

| Base Class | File | Meaning |
|-----------|------|---------|
| `ModulatorSynth` | `hi_core/hi_dsp/modules/ModulatorSynth.h:65` | All synth modules |
| `VoiceEffectProcessor` | `hi_core/hi_dsp/modules/EffectProcessor.h:155` | Polyphonic effect processors |
| `HardcodedProcessor` | `hi_core/hi_modules/hardcoded/HardcodedModules.h:97` | Hardcoded DSP modules |

This means RoutingMatrix works with any synth, polyphonic effect, or hardcoded processor -- but NOT with modulators, MIDI processors, or monophonic effects (unless they inherit from one of these base classes through other means).

---

## MatrixData Inner Class

File: `hi_core/hi_dsp/Routing.h`, lines 63-169; `Routing.cpp`, full implementation

### Data Layout

```cpp
// Fixed-size arrays, all NUM_MAX_CHANNELS (16) elements
int channelConnections[NUM_MAX_CHANNELS];   // source -> destination mapping (-1 = unconnected)
int sendConnections[NUM_MAX_CHANNELS];      // source -> send destination mapping (-1 = unconnected)
float sourceGainValues[NUM_MAX_CHANNELS];   // current peak values for source channels
float targetGainValues[NUM_MAX_CHANNELS];   // current peak values for target channels
int numEditors[NUM_MAX_CHANNELS];           // reference count for editor visibility

int numSourceChannels;        // defaults to 2
int numDestinationChannels;   // defaults to 2
int numAllowedConnections;    // constrains max active connections
bool resizeAllowed;           // defaults to false
bool allowEnablingOnly;       // when true, only enable/disable (no routing)
```

### Channel Limit

`NUM_MAX_CHANNELS` is defined in `hi_tools/Macros.h` as **16**. All internal arrays use this fixed size. The `setNumChannels` script method validates `isPositiveAndBelow(numSourceChannels, NUM_MAX_CHANNELS + 1)`, so valid range is 0-16.

### HISE_NUM_PLUGIN_CHANNELS

Defined in `hi_core/LibConfig.h` as **2** (default). This affects `addConnection` on the MainSynthChain -- if the destination channel exceeds `getNumDestinationChannels()` but is within `HISE_NUM_PLUGIN_CHANNELS`, it's still allowed. This is a special case only for the top-level synth chain.

### Connection Model

Each source channel maps to exactly ONE destination channel (stored in `channelConnections[source]`). A value of `-1` means unconnected. Multiple source channels CAN map to the same destination (many-to-one), but each source has at most one destination (one-to-one from source perspective).

Send connections use an identical parallel array (`sendConnections[source]`) with the same semantics.

### numAllowedConnections Constraint

When `numAllowedConnections == 2` (the default stereo case), the matrix enforces at most 2 active connections:

- `addConnection`: If adding would exceed 2 connections, it removes old connections on even or odd channels depending on which side the new connection is on
- `removeConnection`: If removal leaves fewer than 2 connections, it auto-restores a default connection (`channelConnections[index] = index`)

This auto-correction behavior means the matrix maintains at least a stereo pair when constrained.

### resetToDefault Behavior

```cpp
void RoutableProcessor::MatrixData::resetToDefault()
{
    // Clear all connections
    for (int i = 0; i < NUM_MAX_CHANNELS; i++)
    {
        channelConnections[i] = -1;
        sendConnections[i] = -1;
    }
    // Set default stereo passthrough
    channelConnections[0] = 0;
    channelConnections[1] = 1;

    // Clear gain values
    FloatVectorOperations::clear(targetGainValues, NUM_MAX_CHANNELS);
    FloatVectorOperations::clear(sourceGainValues, NUM_MAX_CHANNELS);

    refreshSourceUseStates();
}
```

Default state: stereo passthrough (source 0 -> dest 0, source 1 -> dest 1), all other channels unconnected.

### clear() Script Method Behavior

The script's `clear()` method does MORE than just `resetToDefault()`:

```cpp
void ScriptRoutingMatrix::clear()
{
    if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
    {
        r->getMatrix().resetToDefault();      // sets default stereo passthrough
        r->getMatrix().removeConnection(0, 0); // then removes the defaults
        r->getMatrix().removeConnection(1, 1);
    }
}
```

This results in ALL connections removed (truly empty matrix), unlike `resetToDefault()` which leaves stereo passthrough. However, note that `removeConnection` with `numAllowedConnections == 2` will auto-restore a default, so the final state may still have connections depending on the constraint setting.

---

## Peak Metering System

### getGainValue

```cpp
float MatrixData::getGainValue(int channelIndex, bool getSource) const
{
    if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getLock()))
    {
        auto ptr = getSource ? sourceGainValues : targetGainValues;
        int numValues = (getSource ? numSourceChannels : numDestinationChannels);
        if (isPositiveAndBelow(channelIndex, numValues))
            return ptr[channelIndex];
    }
    return 0.0f;
}
```

The script method `getSourceGainValue(channelIndex)` calls `getGainValue(channelIndex, true)` -- always reads source (input) gain values.

### handleDisplayValues

Peak values are updated by `handleDisplayValues()` which is called from the audio processing chain. It only computes peaks when `anyChannelActive()` returns true, which requires at least one editor to be shown (via `setEditorShown`). This means **peak values are zero unless the routing editor is actively displayed**.

The `setForcePeakMeters` method listed in the base JSON does NOT exist in the C++ source. It is not in the header, not in the implementation, and not registered in the constructor. This appears to be a phantom method.

### Decay Coefficients

The peak metering system supports smooth decay via `upDecayFactor` (default 1.0) and `downDecayFactor` (default 0.97). These create smooth visual metering when the routing editor is displayed.

---

## Threading and Locking

### SimpleReadWriteLock

MatrixData uses `SimpleReadWriteLock` for thread safety:
- **Write lock** acquired by: `addConnection`, `removeConnection`, `addSendConnection`, `removeSendConnection`, `toggleConnection`, `toggleEnabling`, `toggleSendEnabling`, `setNumSourceChannels`
- **Try-read lock** used by: `getGainValue`, `setGainValues` (non-blocking)

The script wrapper methods (`ScriptRoutingMatrix::addConnection`, etc.) do NOT acquire additional locks -- they delegate directly to the MatrixData methods which handle their own locking.

### refreshSourceUseStates

Every connection change triggers `refreshSourceUseStates()` which:
1. Updates the owning processor's `leftSourceChannel`/`rightSourceChannel`/`leftTargetChannel`/`rightTargetChannel` shortcut fields
2. Calls `owningProcessor->connectionChanged()` (virtual, empty default)
3. Calls `sendChangeMessage()` (SafeChangeBroadcaster notification)

The `sendChangeMessage()` is what allows Broadcaster's `attachToRoutingMatrix` to receive notifications.

---

## Broadcaster Integration

File: `ScriptBroadcaster.h` line 589, `ScriptBroadcaster.cpp` lines 1763-1903

The `Broadcaster.attachToRoutingMatrix(moduleIds, metadata)` method creates a `RoutingMatrixListener` that:

1. Creates a `MatrixListener` (inherits `SafeChangeListener`) for each processor
2. Each `MatrixListener` adds itself as a change listener on the processor's MatrixData
3. When a connection changes, `changeListenerCallback` fires and sends `[processorId, scriptMatrix]` as async message args
4. The callback receives two arguments: processor ID (String) and the RoutingMatrix object itself
5. Initial calls fire once per watched processor with its current state

This is how script code can reactively respond to routing changes without polling.

---

## Serialization (ValueTree)

```cpp
ValueTree MatrixData::exportAsValueTree() const
{
    ValueTree v("RoutingMatrix");
    v.setProperty("NumSourceChannels", numSourceChannels, nullptr);
    for (int i = 0; i < getNumSourceChannels(); i++)
    {
        v.setProperty("Channel" + String(i), channelConnections[i], nullptr);
        v.setProperty("Send" + String(i), sendConnections[i], nullptr);
    }
    return v;
}
```

Properties: `NumSourceChannels`, `Channel0`..`ChannelN`, `Send0`..`SendN`. The `RESTORE_MATRIX()` macro reads the `RoutingMatrix` child from a processor's ValueTree.

---

## setNumChannels Details

```cpp
void ScriptRoutingMatrix::setNumChannels(int numSourceChannels)
{
    if (!isPositiveAndBelow(numSourceChannels, NUM_MAX_CHANNELS + 1))
    {
        reportScriptError("illegal channel amount: " + String(numSourceChannels));
        RETURN_VOID_IF_NO_THROW();
    }

    if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
    {
        if (!r->getMatrix().resizingIsAllowed())
        {
            reportScriptError("Can't resize this matrix");
            RETURN_VOID_IF_NO_THROW();
        }

        r->getMatrix().setNumSourceChannels(numSourceChannels);
        r->getMatrix().setNumAllowedConnections(numSourceChannels);
    }
}
```

Two validation gates:
1. Channel count must be 0-16 (NUM_MAX_CHANNELS)
2. The matrix must have `resizeAllowed == true`

Note: This sets BOTH `numSourceChannels` and `numAllowedConnections` to the same value. It does NOT change `numDestinationChannels`.

---

## Query Methods: Polymorphic Return Types

### getSourceChannelsForDestination(destinationIndex)

- Accepts a single int OR an array of ints
- For a single destination: returns -1 (no source), a single int (one source), or an Array (multiple sources map to same destination)
- For an array input: returns an array of results (recursive call per element)

### getDestinationChannelForSource(sourceIndex)

- Accepts a single int OR an array of ints
- For a single source: returns -1 (not connected) or the destination channel int
- For an array input: returns an array of results (recursive call per element)

This polymorphic behavior (accept single value or array, return matching shape) is a common HISE scripting API pattern.

---

## setForcePeakMeters -- Phantom Method

The method `setForcePeakMeters(bool shouldBeEnabled)` appears in the base JSON but does NOT exist anywhere in the HISE C++ source:
- Not in `ScriptingApiObjects.h` class declaration
- Not in `ScriptingApiObjects.cpp` implementation
- Not registered in the constructor
- Not in the Wrapper struct
- A grep across the entire HISE repository returns only the base JSON file itself

This is either a planned but unimplemented method, or was removed at some point. The underlying peak metering system relies on `anyChannelActive()` which checks `numEditors` reference counts, not a force flag.

---

## Presets Enum (Not Exposed to Script)

The `RoutableProcessor::Presets` enum is used only by the routing editor UI:

| Value | Name | Behavior |
|-------|------|----------|
| 10000 | AllChannels | Connect each source to same-index destination |
| 10001 | FirstStereo | Only channels 0->0, 1->1 |
| 10002 | SecondStereo | Only channels 2->2, 3->3 |
| 10003 | ThirdStereo | Only channels 4->4, 5->5 |
| 10004 | AllChannelsToStereo | All sources map to dest 0 (even) or 1 (odd) |

These are NOT exposed to the scripting API but can be replicated with addConnection/removeConnection calls.

---

## Preprocessor Guards

- `#if USE_BACKEND` guards the routing editor popup (`editRouting`) and the `MatrixViewer` component in the Broadcaster's `RoutingMatrixListener::registerSpecialBodyItems`
- No other preprocessor guards affect the scripting API surface
