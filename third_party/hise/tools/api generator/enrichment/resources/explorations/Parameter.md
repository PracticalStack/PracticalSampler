# Parameter -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- Parameter entry (scriptnode, handle, createdBy: Node, creates: Connection)
- `enrichment/phase1/Node/Readme.md` -- Node class analysis (prerequisite)
- `enrichment/phase1/DspNetwork/Readme.md` -- DspNetwork class analysis (prerequisite)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/scriptnode/api/NodeBase.h`, line 119

```cpp
class Parameter : public ConstScriptingObject
```

Parameter is declared in the `scriptnode` namespace, inside the same header as `NodeBase`. It inherits from `ConstScriptingObject` (not `ApiClass`), consistent with other scriptnode scripting objects.

### Key Members

| Member | Type | Description |
|--------|------|-------------|
| `parent` | `NodeBase*` | Owning node (public) |
| `data` | `ValueTree` | Parameter's ValueTree (public) |
| `valueNames` | `StringArray` | Named values for discrete parameters (public) |
| `isProbed` | `bool` | Debugging flag (public) |
| `dynamicParameter` | `parameter::dynamic_base::Ptr` | The DSP callback that actually processes value changes (private) |
| `connectionSourceTree` | `ValueTree` | Cached source connection tree (private) |
| `rangeListener` | `valuetree::PropertyListener` | Monitors range property changes (private) |
| `valuePropertyUpdater` | `valuetree::PropertyListener` | Monitors Value property changes (private) |
| `idUpdater` | `valuetree::PropertyListener` | Monitors ID changes (private) |
| `modulationStorageBypasser` | `valuetree::PropertyListener` | Bypasses modulation storage (private) |
| `automationRemover` | `valuetree::RemoveListener` | Cleans up automation on removal (private) |

### getObjectName

Returns `PropertyIds::Parameter`, which is the JUCE Identifier `"Parameter"`.

## Constructor

**File:** `NodeBase.cpp`, line 775

```cpp
Parameter::Parameter(NodeBase* parent_, const ValueTree& data_) :
    ConstScriptingObject(parent_->getScriptProcessor(), 4),
    parent(parent_),
    data(data_),
    dynamicParameter()
```

The `4` passed to `ConstScriptingObject` is the number of constants.

### API Method Registration

All methods use plain `ADD_API_METHOD_N` (no typed variants):

```cpp
ADD_API_METHOD_0(getValue);
ADD_API_METHOD_1(addConnectionFrom);
ADD_API_METHOD_1(setValueAsync);
ADD_API_METHOD_1(setValueSync);
ADD_API_METHOD_2(setRangeProperty);
ADD_API_METHOD_0(getId);
ADD_API_METHOD_1(setRangeFromObject);
ADD_API_METHOD_0(getRangeObject);
```

### Constants

Four constants registered via macro:

```cpp
#define ADD_PROPERTY_ID_CONSTANT(id) addConstant(id.toString(), id.toString());

ADD_PROPERTY_ID_CONSTANT(PropertyIds::MinValue);
ADD_PROPERTY_ID_CONSTANT(PropertyIds::MaxValue);
ADD_PROPERTY_ID_CONSTANT(PropertyIds::MidPoint);
ADD_PROPERTY_ID_CONSTANT(PropertyIds::StepSize);
```

These are string-to-string constants. Each constant's name AND value is the PropertyId string itself. They provide convenient access to range property IDs for use in `setRangeProperty()`.

| Constant Name | Value (String) | Purpose |
|---------------|----------------|---------|
| `MinValue` | `"MinValue"` | Minimum value of the parameter range |
| `MaxValue` | `"MaxValue"` | Maximum value of the parameter range |
| `MidPoint` | `"MidPoint"` | Skew midpoint of the range |
| `StepSize` | `"StepSize"` | Step interval for discrete values |

Note: `SkewFactor` and `Inverted` are NOT registered as constants, even though they are valid range properties in the ValueTree. This means `setRangeProperty()` can accept them as string literals but they don't have constant shortcuts on the Parameter object.

### ValueTree Listeners

Three listeners are set up in the constructor:

1. **valuePropertyUpdater**: Watches `PropertyIds::Value` synchronously. Calls `updateFromValueTree()` which forwards to `setValueAsync()`. This is the bridge from ValueTree changes to DSP parameter updates.

2. **rangeListener**: Watches all range IDs (`RangeHelpers::getRangeIds()`) synchronously. Calls `updateRange()` which forwards to `dynamicParameter->updateRange(data)`.

3. **automationRemover**: A `RemoveListener` that watches for child removal. Calls `updateConnectionOnRemoval()` which removes the `connectionSourceTree` from its parent when the parameter is removed (unless `ScopedAutomationPreserver` is active).

## Wrapper Struct

**File:** `NodeBase.cpp`, line 763

```cpp
struct Parameter::Wrapper
{
    API_METHOD_WRAPPER_0(NodeBase::Parameter, getId);
    API_METHOD_WRAPPER_0(NodeBase::Parameter, getValue);
    API_VOID_METHOD_WRAPPER_2(NodeBase::Parameter, setRangeProperty);
    API_METHOD_WRAPPER_1(NodeBase::Parameter, addConnectionFrom);
    API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setValueSync);
    API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setValueAsync);
    API_VOID_METHOD_WRAPPER_1(NodeBase::Parameter, setRangeFromObject);
    API_METHOD_WRAPPER_0(NodeBase::Parameter, getRangeObject);
};
```

No `ADD_TYPED_API_METHOD_N` calls -- all methods use untyped wrappers.

## Inner Types

### ScopedAutomationPreserver

**File:** `NodeBase.h`, line 126

RAII guard that prevents automation connection cleanup when a node is being moved (e.g., during drag operations). Sets a flag on the parent NodeBase and restores it on destruction. The static `isPreservingRecursive()` checks the flag up the parent chain.

```cpp
struct ScopedAutomationPreserver
{
    ScopedAutomationPreserver(NodeBase* n);
    ~ScopedAutomationPreserver();
    static bool isPreservingRecursive(NodeBase* n);
private:
    NodeBase* parent;
    bool prevValue;
};
```

## Obtained Via

Parameter objects are obtained from Node instances through:

1. **`Node.getOrCreateParameter(indexOrId)`** -- Returns existing parameter by index (int) or name (string), or creates a new one on container nodes from a JSON descriptor.

2. **`Node.getParameterFromName(id)`** -- Internal lookup by name (not a scripting API method).

3. **`Connection.getTarget()`** -- Returns the target Parameter of a connection.

Parameters are created internally during node construction from the node's parameter ValueTree. Container nodes can also dynamically create parameters via `getOrCreateParameter()`.

## The dynamic_base Parameter Callback System

### parameter::dynamic_base

**File:** `HISE/hi_scripting/scripting/scriptnode/api/DynamicProperty.h`, line 44

This is the core callback mechanism. Each Parameter holds a `dynamic_base::Ptr` which is the actual DSP callback invoked when the parameter value changes.

```cpp
struct dynamic_base: public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<dynamic_base>;
    
    virtual void call(double value);
    virtual double getDisplayValue() const;
    virtual InvertableParameterRange getRange() const;
    virtual void updateRange(const ValueTree& v);
    
protected:
    void setDisplayValue(double v);
    
private:
    InvertableParameterRange range;
    double lastValue = 0.0;
};
```

Key points:
- `call(double)` is the hot path -- invoked on audio thread to set the DSP parameter
- `getDisplayValue()` returns the last value set (for UI display)
- `updateRange()` refreshes the range from ValueTree data
- `createFromConnectionTree()` is a static factory that creates the appropriate `dynamic_base` subclass from a connection ValueTree

### parameter::dynamic_base_holder

**File:** `DynamicProperty.h`, line 82

A holder wrapper around `dynamic_base` that adds thread-safe swapping via `SimpleReadWriteLock` and forwarding semantics. Used when a parameter needs to swap its underlying callback at runtime (e.g., when connections change).

### setDynamicParameter()

**File:** `NodeBase.cpp`, line 1040

```cpp
void Parameter::setDynamicParameter(parameter::dynamic_base::Ptr ownedNew)
{
    bool useLock = parent->isActive(true) && parent->getRootNetwork()->isInitialised();
    auto ph = parent->getRootNetwork()->getParentHolder();
    if(ph == nullptr) return;
    
    SimpleReadWriteLock::ScopedWriteLock sl(
        parent->getRootNetwork()->getConnectionLock(), useLock);
    
    dynamicParameter = ownedNew;
    
    if (dynamicParameter != nullptr)
    {
        dynamicParameter->updateRange(data);
        if (data.hasProperty(PropertyIds::Value))
            dynamicParameter->call((double)data[PropertyIds::Value]);
    }
}
```

This acquires the network's connection write lock only if the network is already active and initialized. After assignment, it immediately syncs the range and calls with the current value.

## Range System

### RangeHelpers

**File:** `HISE/hi_dsp_library/node_api/helpers/ParameterData.h`, line 72

The range system uses `InvertableParameterRange` which wraps a JUCE `NormalisableRange<double>` with an additional `inv` (inverted) flag.

Range property IDs for scriptnode (the default `IdSet::scriptnode`):
- `MinValue` -- range start
- `MaxValue` -- range end
- `StepSize` -- interval
- `SkewFactor` -- skew factor
- `MidPoint` -- midpoint (used for UI, stored in ValueTree)
- `Inverted` -- whether the range is inverted

`RangeHelpers::getRangeIds()` returns the array of Identifiers used for the scriptnode IdSet: `[MinValue, MaxValue, StepSize, SkewFactor]`. When `includeValue=true`, it also includes `Value`.

`RangeHelpers::isRangeId()` checks if an Identifier is one of the range property IDs -- used by `setRangeProperty()` to validate the property name.

### getRangeObject() Implementation

Returns a `DynamicObject` with five properties:
- `MinValue` -- from range start
- `MaxValue` -- from range end
- `SkewFactor` -- from range skew
- `StepSize` -- from range interval
- `Inverted` -- from InvertableParameterRange.inv

### setRangeFromObject() Implementation

Reads from JSON with defaults:
- `MinValue` defaults to `0.0`
- `MaxValue` defaults to `1.0`
- `SkewFactor` defaults to `1.0`
- `StepSize` defaults to `0.0`
- `Inverted` defaults to `false`

Calls `checkIfIdentity()` on the range, then stores to ValueTree via `RangeHelpers::storeDoubleRange()` with undo support.

### setRangeProperty() Implementation

Validates the property ID via `RangeHelpers::isRangeId()`, then sets the property on the ValueTree. Note: this uses `nullptr` for the UndoManager (no undo support), unlike `setRangeFromObject()` which passes `parent->getUndoManager()`.

## Value Setting: Async vs Sync

### setValueAsync()

**File:** `NodeBase.cpp`, line 1063

```cpp
void Parameter::setValueAsync(double newValue)
{
    if (dynamicParameter != nullptr)
    {
        DspNetwork::NoVoiceSetter nvs(*parent->getRootNetwork());
        dynamicParameter->call(newValue);
    }
}
```

- Sets value immediately via the dynamic parameter's `call()` method
- Uses `NoVoiceSetter` RAII wrapper which scopes `PolyHandler::ScopedAllVoiceSetter` -- this ensures the value applies to ALL voices in polyphonic networks
- Does NOT store to ValueTree -- "asynchronously" refers to the ValueTree update being deferred
- If `dynamicParameter` is nullptr (parameter not yet connected to DSP), the call is silently ignored

### setValueSync()

**File:** `NodeBase.cpp`, line 1112

```cpp
void Parameter::setValueSync(double newValue)
{
    data.setProperty(PropertyIds::Value, newValue, parent->getUndoManager());
}
```

- Stores value to ValueTree synchronously with undo support
- The ValueTree change triggers `valuePropertyUpdater` listener, which calls `updateFromValueTree()`, which calls `setValueAsync()` -- so the DSP callback is eventually invoked
- The chain: `setValueSync()` -> ValueTree change -> `updateFromValueTree()` -> `setValueAsync()` -> `dynamicParameter->call()`

### Key Difference

| Aspect | setValueAsync | setValueSync |
|--------|---------------|--------------|
| DSP update | Immediate | Deferred (via listener) |
| ValueTree update | Not performed | Immediate with undo |
| Undo support | No | Yes |
| Voice scoping | All voices (NoVoiceSetter) | Via the async path |
| Use case | Runtime automation, modulation | UI interaction, preset recall |

## Connection System (addConnectionFrom)

### DragHelpers

**File:** `NodeBase.cpp`, line 1188

Local struct providing static helpers for parsing connection drag data:

```cpp
struct DragHelpers
{
    static var createDescription(const String& sourceNodeId, const String& parameterId, bool isMod=false);
    static String getSourceNodeId(var dragDetails);
    static String getSourceParameterId(var dragDetails);
    static ModulationSourceNode* getModulationSource(NodeBase* parent, var dragDetails);
    static ValueTree getValueTreeOfSourceParameter(NodeBase* parent, var dragDetails);
};
```

The `connectionData` argument to `addConnectionFrom()` can be:
1. **A JSON object** with `ID` (source node ID), `ParameterId` (source parameter), and optional `Automated` flag -- adds a connection
2. **A string** in format `"nodeId"` -- used to look up modulation source nodes directly
3. **Any non-object value** (including undefined/null) -- removes the existing connection

### addConnectionFrom() Logic

When `connectionData` is an object (add mode):
1. If parameter is already `Automated`, returns empty var (no duplicate connections)
2. Sets `Automated = true` on the parameter's ValueTree
3. Checks if the source is a ModulationSourceNode -- if so, calls `modSource->addModulationConnection(0, this)`
4. Checks for self-connection (source == target) and rejects
5. Otherwise calls `sn->addModulationConnection(parameterId, this)` on the source node

When `connectionData` is not an object (remove mode):
1. Gets the `connectionSourceTree` via `getConnectionSourceTree(true)` (force refresh)
2. Sets `Automated = false`
3. Removes the connection from the ValueTree with undo

### Return Value

Returns a `var` -- when adding from a ModulationSourceNode, this returns the new Connection object (a `ConnectionBase` scripting wrapper). When adding from a container parameter, returns the result of `addModulationConnection()`. When removing, returns empty var.

## isModulated()

**File:** `NodeBase.h`, line 205

```cpp
bool isModulated() const 
{ 
    return (bool)data.getProperty(PropertyIds::Automated, false);
}
```

Checks the `Automated` property on the parameter's ValueTree. Not a scripting API method -- internal only.

## getConnectionSourceTree()

**File:** `NodeBase.cpp`, line 1117

When `forceUpdate=true`, traverses the entire network searching for the connection that targets this parameter:
1. Checks all container nodes' parameter connection trees
2. Checks all wrapper nodes' ModulationTargets trees
3. Checks all wrapper nodes' SwitchTargets trees

Returns the matching ValueTree and caches it in `connectionSourceTree`.

## getConnectedMacroParameters()

**File:** `NodeBase.cpp`, line 1401

Walks up the parent chain looking for `NodeContainer::MacroParameter` instances that are connected to this parameter. Used internally for UI display and connection management.

## Threading and Lifecycle

### Value Setting Thread Safety

- `setValueAsync()` uses `NoVoiceSetter` but does NOT acquire any lock -- it directly calls `dynamicParameter->call()`. This is safe because:
  - The dynamic_base's `call()` is a simple function pointer dispatch (lock-free)
  - Connection changes (which swap `dynamicParameter`) acquire the network's `connectionLock` write lock
  - Audio processing uses a `ScopedTryReadLock` on the same lock

- `setValueSync()` modifies the ValueTree which operates on the message thread. The listener chain ensures the value eventually reaches the DSP callback.

- `setDynamicParameter()` acquires the network's `connectionLock` as a write lock only when the network is active. This is the only point where `dynamicParameter` is swapped.

### ValueTree Listener Modes

All three listeners use `valuetree::AsyncMode::Synchronously`, meaning they fire immediately when the property changes (not deferred to the message thread). This ensures DSP parameter updates are immediate.

## Preprocessor Guards

No preprocessor guards found in the Parameter class itself. The class is always compiled and available in all build targets (backend, frontend, DLL).

## ConnectionBase -- Created by addConnectionFrom()

**File:** `NodeBase.h`, line 713

`ConnectionBase` is the scripting API wrapper returned by `addConnectionFrom()`. It represents a single connection between a source and a target parameter.

```cpp
class ConnectionBase final: public ConstScriptingObject
{
public:
    enum ConnectionSource
    {
        MacroParameter,           // 0 -- from container macro parameter
        SingleOutputModulation,   // 1 -- from single-output mod node
        MultiOutputModulation,    // 2 -- from multi-output mod node
        numConnectionSources
    };
    
    // API methods:
    var getTarget() const;
    var getSourceNode(bool getSignalSource) const;
    void disconnect();
    int getConnectionType() const;
    int getUpdateRate() const;
    bool isConnected() const;
};
```

## ValueTree Structure

A Parameter's ValueTree has type `Parameter` and contains:

| Property | Type | Description |
|----------|------|-------------|
| `ID` | String | Parameter name |
| `Value` | double | Current value |
| `MinValue` | double | Range minimum |
| `MaxValue` | double | Range maximum |
| `StepSize` | double | Step interval |
| `SkewFactor` | double | Range skew |
| `MidPoint` | double | Skew midpoint |
| `Inverted` | bool | Whether range is inverted |
| `Automated` | bool | Whether connected to a source |

Child tree:
- `Connections` -- contains Connection ValueTrees linking to sources

This matches the structure documented in the Node exploration's `node-valuetree-structure` diagram.
