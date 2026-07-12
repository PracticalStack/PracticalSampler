# Node (NodeBase) -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- Node entry (line 1534)
- `enrichment/resources/survey/class_survey.md` -- prerequisite row 7: DspNetwork -> Node
- `enrichment/phase1/DspNetwork/Readme.md` -- prerequisite context
- `enrichment/resources/explorations/DspNetwork.md` -- existing exploration (referenced, not re-read)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/scriptnode/api/NodeBase.h:232`

```cpp
class NodeBase : public ConstScriptingObject,
                 public ObjectWithJSONConverter,
                 public ParameterSourceObject
```

- **Namespace:** `scriptnode`
- **Scripting name:** `Node` (returned by `getObjectName()` as `PropertyIds::Node`)
- **Base classes:**
  - `ConstScriptingObject` -- standard scripting API object with constant slots
  - `ObjectWithJSONConverter` -- provides `writeAsJSON()` for ValueTree-to-JSON serialization
  - `ParameterSourceObject` -- provides `getLastPrepareSpecs()`, `getParameterValue()`, `getParameterRange()`, `getValueTree()`, `getUndoManager()` interface
- **Pure virtual methods:** `process(ProcessDataDyn&)`, `processFrame(FrameType&)`, `reset()`
- **Key type aliases:**
  - `Ptr = WeakReference<NodeBase>`
  - `List = Array<WeakReference<NodeBase>>`
  - `FrameType = snex::Types::dyn<float>`
  - `MonoFrameType = snex::Types::span<float, 1>`
  - `StereoFrameType = snex::Types::span<float, 2>`

## Inheritance Hierarchy (Downstream)

NodeBase is the base class for all scriptnode processing nodes. Key subclasses:

- **WrapperNode** -- wraps opaque DSP objects, provides extra component UI. Subclass of NodeBase.
  - **ModulationSourceNode** -- nodes that output modulation signals (extends WrapperNode + ConnectionSourceManager)
- **NodeContainer** (mix-in struct, not a NodeBase subclass itself but used alongside):
  - **SerialNode** (NodeBase + NodeContainer) -- serial signal chain
    - **SoftBypassNode** -- serial container with soft bypass capability (required for dynamic bypass connections)
  - **ParallelNode** (NodeBase + NodeContainer) -- parallel signal chain

## Inner Classes and Structs

### NodeBase::Holder
```cpp
struct Holder {
    NodeBase* getRootNode() const;
    void setRootNode(NodeBase::Ptr newRootNode);
    ReferenceCountedObjectPtr<NodeBase> root;
    ReferenceCountedArray<NodeBase> nodes;
};
```
Manages the root node and flat node list. DspNetwork implements Holder. The `getNodeHolder()` API method returns the holder -- which may be the DspNetwork or a sub-holder (stored in `subHolder` member).

### NodeBase::DynamicBypassParameter
```cpp
struct DynamicBypassParameter : public parameter::dynamic_base {
    void call(double v) final override;
    Range<double> enabledRange;
};
```
Created when a bypass connection is established. The `call()` method checks if the value is within the `enabledRange` -- if outside the range, the node is bypassed. Includes `ScopedUndoDeactivator` to suppress undo during bypass state changes.

### NodeBase::ParameterIterator
RAII iterator for the parameters array. Used internally for `for (auto p : ParameterIterator(node))` loops.

### NodeBase::ScopedUndoDeactivator
Sets `returnIfPending = true` during its lifetime, which causes `getUndoManager()` to return nullptr when an undo operation is pending.

### NodeBase::Wrapper (in .cpp)
Standard API method wrapper struct. All methods use untyped `ADD_API_METHOD_N` (no typed variants).

## Constructor

**File:** `NodeBase.cpp:81`

```cpp
NodeBase::NodeBase(DspNetwork* rootNetwork, ValueTree data_, int numConstants_) :
    ConstScriptingObject(rootNetwork->getScriptProcessor(), 8),
    parent(rootNetwork),
    v_data(data_),
    helpManager(this, data_),
    currentId(v_data[PropertyIds::ID].toString()),
    subHolder(rootNetwork->getCurrentHolder()),
    profileData(new DebugSession::ProfileDataSource())
```

Key observations:
- **8 constant slots** allocated via `ConstScriptingObject(proc, 8)`
- **No `addConstant()` calls with fixed values** -- constants are dynamically added from the property tree:
  ```cpp
  for (auto c : getPropertyTree())
      addConstant(c[PropertyIds::ID].toString(), c[PropertyIds::ID]);
  ```
  This means each node's constants are the string IDs of its node-specific properties (e.g., "Mode", "Frequency", etc.). These are NOT fixed across all Node instances -- they depend on the specific node type.

### API Method Registration (Constructor)

All registered via plain `ADD_API_METHOD_N` (no typed variants):

| Registration | Method | Args |
|---|---|---|
| `ADD_API_METHOD_0` | `reset` | 0 |
| `ADD_API_METHOD_2` | `set` | 2 |
| `ADD_API_METHOD_1` | `get` | 1 |
| `ADD_API_METHOD_1` | `setBypassed` | 1 |
| `ADD_API_METHOD_0` | `isBypassed` | 0 |
| `ADD_API_METHOD_2` | `setParent` | 2 |
| `ADD_API_METHOD_1` | `getOrCreateParameter` | 1 |
| `ADD_API_METHOD_1` | `getParameter` | 1 |
| `ADD_API_METHOD_2` | `connectTo` | 2 |
| `ADD_API_METHOD_1` | `connectToBypass` | 1 |
| `ADD_API_METHOD_3` | `setComplexDataIndex` | 3 |
| `ADD_API_METHOD_0` | `getNumParameters` | 0 |
| `ADD_API_METHOD_1` | `getChildNodes` | 1 |

### Methods in Base JSON but NOT Registered

Three methods appear in the base JSON (from Doxygen extraction) but are NOT registered via ADD_API_METHOD:

1. **`getIndexInParent`** -- public method with doc comment, within the NODE API section, but no ADD_API_METHOD_0 call. Implementation just returns `v_data.getParent().indexOf(v_data)`.
2. **`isActive`** -- public method with doc comment, within the NODE API section, but no ADD_API_METHOD_1 call. Checks ValueTree parent chain up to Network type.
3. **`getNodeHolder`** -- public method with doc comment, OUTSIDE the NODE API section (below END NODE API). Returns the `subHolder` or the parent DspNetwork.

These methods exist in C++ but may or may not be callable from HISEScript. The Wrapper struct does NOT include wrappers for them either.

### Registered but NOT in Base JSON

- **`getParameter`** -- has both a Wrapper entry and ADD_API_METHOD_1 registration, but does NOT appear in the base JSON. It is a separate method from `getOrCreateParameter` that only retrieves existing parameters (no creation).

## ValueTree Structure

Each node is backed by a JUCE ValueTree with this structure:

```
Node (type: PropertyIds::Node)
  +-- ID: String (unique node identifier)
  +-- Name: String (display name, defaults to ID)
  +-- FactoryPath: String (e.g. "core.gain", "container.chain")
  +-- Bypassed: bool (default false)
  +-- NodeColour: int (default 0)
  +-- Comment: String (default "")
  +-- Folded: bool (collapsed in UI)
  +-- Parameters/ (child tree)
  |   +-- Parameter (one per parameter)
  |       +-- ID, Value, MinValue, MaxValue, SkewFactor, StepSize, ...
  |       +-- Connections/ (child tree for outgoing connections)
  +-- Properties/ (child tree, node-type-specific)
  |   +-- Property
  |       +-- ID: String
  |       +-- Value: var
  +-- Nodes/ (child tree, only for containers)
  +-- ComplexData/ (child tree, only for data-consuming nodes)
  |   +-- Tables/
  |   +-- SliderPacks/
  |   +-- AudioFiles/
  |   +-- ...
  +-- ModulationTargets/ (for ModulationSourceNode)
  +-- SwitchTargets/ (for switch nodes)
```

## Property System (get/set methods)

The `set(id, value)` method handles two property locations:

1. **Node properties** (in the Properties child tree): checked via `hasNodeProperty()`, set via `setNodeProperty()`. These are node-type-specific properties like "Mode", "Frequency", etc.
2. **ValueTree properties** (direct properties on the node tree): checked via `getValueTree().hasProperty()`, set via `setProperty()`. These are standard properties like Bypassed, NodeColour, Comment, etc.

The `get(id)` method ONLY reads from node properties (the Properties child tree), NOT from direct ValueTree properties.

**Important asymmetry:** `set()` can write to both locations; `get()` only reads node properties.

## Bypass System

### Simple bypass (`setBypassed`/`isBypassed`)
- `setBypassed(bool)` is virtual -- sets `bypassState` member. The actual bypassing behavior is implemented by subclasses that override `process()`/`processFrame()`.
- `isBypassed()` reads from the `bypassState` member, NOT from the ValueTree directly.
- A `bypassListener` (valuetree::PropertyListener) watches `PropertyIds::Bypassed` on the ValueTree and calls `updateBypassState()` -> `setBypassed()` to sync.

### Dynamic bypass (`connectToBypass`)
- Creates a ValueTree Connection entry targeting `PropertyIds::Bypassed` as the parameterId.
- Can connect from: a container macro parameter, a modulation source node, or a switch target.
- **Constraint:** Dynamic bypass connections require the target node to be a `SoftBypassNode` (checked in `createParameterFromConnectionTree`). If not, an `IllegalBypassConnection` error is raised.
- When called with no new source (toggle behavior), it searches for and removes existing bypass connections.
- The `DynamicBypassParameter` class handles the actual bypass logic: it checks if the incoming value is within the `enabledRange` and calls `setBypassed()` accordingly.

## Connection System (connectTo)

The `connectTo(parameterTarget, sourceInfo)` method:
1. Casts `parameterTarget` to `Parameter*`
2. Calls `addModulationConnection(sourceInfo, targetParameter)` -- a virtual method
3. The base `NodeBase` implementation returns empty var (no-op)
4. **Overrides:**
   - `SerialNode`/`ParallelNode` -> `NodeContainer::addMacroConnection()` -- connects via container macro parameters
   - `ModulationSourceNode` -> `addModulationConnection()` -- creates modulation target connections

The `sourceInfo` parameter semantics depend on the node type:
- For containers: it is the parameter name (String) of the source macro parameter
- For modulation sources: it is the output slot index (integer)

## setParent Implementation

```cpp
void NodeBase::setParent(var parentNode, int indexInParent)
```
1. Calls `checkValid()` to ensure network is valid
2. Sets `isCurrentlyMoved = true` via ScopedValueSetter
3. If `parentNode` is the DspNetwork itself, converts to root node
4. Creates `ScopedAutomationPreserver` to preserve connections during move
5. Removes from current parent (if any)
6. Finds the target container via `network->get(parentNode)`, casts to `NodeContainer*`
7. Adds to new parent's node tree at the specified index
8. If parent not found and name is non-empty, reports script error
9. If parent is empty/null, removes from current parent (detach operation)

## getOrCreateParameter Implementation

```cpp
var NodeBase::getOrCreateParameter(var indexOrId) const
```
1. First tries `getParameter(indexOrId)` -- returns existing if found
2. If not found and `this` is a `NodeContainer`:
   - Extracts name, range, mode from the input object
   - Creates a new Parameter ValueTree with ID, range properties, defaults
   - Sets `PropertyIds::TextToValueConverter` from `mode` property if present
   - Adds to the Parameters child tree
   - Returns the newly created Parameter
3. If not a container: reports script error "Can't create parameter for non-container node"

**Key insight:** Only container nodes (SerialNode, ParallelNode, etc.) can have parameters created dynamically. Leaf nodes have fixed parameter lists defined by their factory type.

## getChildNodes Implementation

```cpp
var NodeBase::getChildNodes(bool recursive)
```
1. Checks if `this` is a `NodeContainer` via `dynamic_cast`
2. If yes, iterates `getNodeList()` and adds each child as a var
3. If recursive, calls `getChildNodes(true)` on each child and appends results
4. Returns array of Node references
5. If not a container, returns empty array

## setComplexDataIndex Implementation

```cpp
bool NodeBase::setComplexDataIndex(String dataType, int dataSlot, int indexValue)
```
1. Gets the `ComplexData` child from the ValueTree
2. Appends "s" to `dataType` to get the child tree name (e.g., "Table" -> "Tables")
3. Gets child at `dataSlot` index
4. Sets `PropertyIds::Index` to `indexValue`
5. Returns false if any step finds an invalid tree

Valid `dataType` string values (from `ExternalData::DataType` enum):
- `"Table"` -- lookup table data (512 floats)
- `"SliderPack"` -- resizable float array
- `"AudioFile"` -- multichannel audio file
- `"FilterCoefficients"` -- filter display data
- `"DisplayBuffer"` -- FIFO visualization buffer

## reset Method

`reset()` is pure virtual in NodeBase. Each concrete node type implements its own reset logic (clearing internal state, resetting filters, etc.). Called at voice start in polyphonic contexts.

## Relationship to DspNetwork (Prerequisite Context)

From the DspNetwork prerequisite:
- **Creation:** Nodes are created via `DspNetwork::create(factoryPath, nodeId)` or `createAndAdd()`
- **Lookup:** `DspNetwork::get(nodeId)` returns a Node reference; bracket syntax `network["nodeId"]` also works
- **Ownership:** DspNetwork's `Holder` struct owns a flat `ReferenceCountedArray<NodeBase> nodes` and a hierarchical ValueTree
- **Processing:** DspNetwork delegates `processBlock()` to the root node's `process()` method
- **Connection lock:** Audio processing uses `ScopedTryReadLock` on the connection lock; structural changes (setParent, connectTo) take write locks

## Threading Constraints

- `set()`/`get()` operate on ValueTree properties which are thread-safe for read but require message thread for write (undo manager involvement)
- `setBypassed()` sets a simple bool member -- no lock required
- `connectTo()` and `connectToBypass()` modify ValueTree structure -- must be on message thread
- `setParent()` modifies ValueTree structure -- must be on message thread
- `reset()` is called from the audio thread
- `setComplexDataIndex()` modifies ValueTree property via undo manager -- message thread

## Profiling Infrastructure

When `HISE_INCLUDE_PROFILING_TOOLKIT` is defined:
- Each node gets a `DebugSession::ProfileDataSource::Ptr profileData`
- Profile data tracks CPU usage, preferred domain is `CpuUsage`
- `locationString` format: `"ProcessorId.NodeId"`
- `sourceType` is `DebugSession::ProfileDataSource::SourceType::Scriptnode`
- `DspNetworkHeatmapGenerator` traverses the node tree for heatmap visualization

## Signal Peak Tracking

```cpp
span<span<float, NUM_MAX_CHANNELS>, 2> signalPeaks;
```
- Two sets of peaks: pre-signal `[0]` and post-signal `[1]`
- Updated via `setSignalPeaks()` with exponential smoothing (0.5 factor)
- Read via `getSignalPeak(channel, post)`
- Only active in `USE_BACKEND` builds (ProcessDataPeakChecker/FrameDataPeakChecker)

## HelpManager

Each node owns a `HelpManager` instance that:
- Renders markdown comments (from the `Comment` property) alongside the node in the graph
- Uses the node's `NodeColour` for headline styling
- Supports collapsing to a comment button (toggle via `ShowComments` network property)
- Position can be to the right or below the node (below when parent is horizontal SerialNode)
