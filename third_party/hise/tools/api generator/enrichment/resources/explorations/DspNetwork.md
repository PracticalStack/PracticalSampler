# DspNetwork -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- prerequisite row 7: DspNetwork -> Node, Parameter, Connection
- `enrichment/resources/survey/class_survey_data.json` -- DspNetwork entry
- `enrichment/phase1/Engine/Readme.md` -- prerequisite context (Engine is DspNetwork's factory)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/scriptnode/api/DspNetwork.h:143`

```cpp
class DspNetwork : public ConstScriptingObject,
                   public Timer,
                   public AssignableObject,
                   public ControlledObject,
                   public NodeBase::Holder
```

### Inheritance Chain
- **ConstScriptingObject** -- Standard scripting API base. Constructor passes `numConstants=2` but no `addConstant()` calls are made (slots unused).
- **Timer** -- JUCE timer. Used for UndoManager transaction boundaries (`timerCallback` calls `um.beginNewTransaction()` every 1500ms when undo is enabled).
- **AssignableObject** -- Enables bracket-operator access (`network["nodeId"]`). `getCachedIndex` resolves string IDs to node list indices; `getAssignedValue` returns `nodes[index]`.
- **ControlledObject** -- Access to MainController.
- **NodeBase::Holder** -- Owns the flat list of all nodes (`NodeBase::List nodes`) and the root node (`NodeBase::Ptr root`).

### Namespace
`scriptnode` (not `hise`). File uses `using namespace juce; using namespace hise;`.

## Constructor Analysis

**File:** `DspNetwork.cpp:56-236`

```cpp
DspNetwork(ProcessorWithScriptingContent* p, ValueTree data_, bool poly, ExternalDataHolder* dataHolder_=nullptr)
```

### Initialization sequence

1. **TempoSyncer setup** -- hooks `tempoSyncer.ppqFunction` to `mc->getMasterClock().getPPQPos(ts)`, sets `publicModValue`
2. **PostInitFunction** for GlobalRoutingManager -- connects `tempoSyncer.additionalEventStorage` and `tempoSyncer.uuidManager` to the routing manager (deferred because routing manager may not exist yet)
3. **PolyHandler** -- `polyHandler.setTempoSyncer(&tempoSyncer)`, registers as tempo listener
4. **ValueTree property defaults** -- sets defaults on the network ValueTree if not present:
   - `AllowCompilation` = false
   - `AllowPolyphonic` = isPoly
   - `CompileChannelAmount` = 2
   - `ModulationBlockSize` = 0
   - `HasTail` = true
   - `SuspendOnSilence` = false
   - `Version` = "0.0.0"
5. **CachedValue bindings** -- `hasTailProperty` and `canBeSuspendedProperty` bound to ValueTree
6. **ExternalDataHolder** -- uses provided holder or falls back to `dynamic_cast<ExternalDataHolder*>(p)`
7. **Node factory registration** (see below)
8. **Loader and local cable manager** creation
9. **Root node creation** -- `setRootNode(createFromValueTree(true, data.getChild(0), true))`
10. **API method registration** (see below)
11. **SelectionUpdater** creation
12. **UndoManager** enabled/disabled based on build target
13. **Exception resetter** and **sort listener** for ValueTree child changes
14. **Deprecation check**
15. **Post-init functions** and **DynamicParameterModulationProperties** initialization
16. **Root parameter listener** -- updates processor parameter slots when root parameters change

### Node Factory Registration (constructor lines 123-150)

Built-in factories registered in order:

| # | Factory | Identifier | Source |
|---|---------|-----------|--------|
| 1 | NodeContainerFactory | `container` | NodeContainer.h |
| 2 | core::Factory | `core` | core factories |
| 3 | math::Factory | `math` | math nodes |
| 4 | envelope::Factory | `envelope` | envelope nodes |
| 5 | routing::Factory | `routing` | routing nodes |
| 6 | analyse::Factory | `analyse` | analysis nodes |
| 7 | fx::Factory | `fx` | effect nodes |
| 8 | control::Factory | `control` | control nodes |
| 9 | dynamics::Factory | `dynamics` | dynamics nodes |
| 10 | filters::Factory | `filters` | filter nodes |
| 11 | jdsp::Factory | `jdsp` | JUCE DSP nodes |

Build-target-specific factories:
- **Backend (`USE_BACKEND`):** `dll::BackendHostFactory` (loads project DLL nodes) + `TemplateNodeFactory`
- **Frontend (`!USE_BACKEND`):** `hise::FrontendHostFactory` (loads compiled project nodes)

### API Method Registration (constructor lines 163-175)

All use plain `ADD_API_METHOD_N` (no typed variants):

```
ADD_API_METHOD_1(processBlock);
ADD_API_METHOD_2(prepareToPlay);
ADD_API_METHOD_2(create);
ADD_API_METHOD_3(createAndAdd);
ADD_API_METHOD_1(get);
ADD_API_METHOD_1(setForwardControlsToParameters);
ADD_API_METHOD_1(setParameterDataFromJSON);
ADD_API_METHOD_1(createTest);
ADD_API_METHOD_2(clear);
ADD_API_METHOD_2(createFromJSON);
ADD_API_METHOD_0(undo);
```

Commented-out methods: `disconnectAll`, `injectAfter`.

### Wrapper Struct (lines 39-54)

```cpp
struct DspNetwork::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(DspNetwork, processBlock);
    API_VOID_METHOD_WRAPPER_2(DspNetwork, prepareToPlay);
    API_METHOD_WRAPPER_2(DspNetwork, create);
    API_VOID_METHOD_WRAPPER_2(DspNetwork, clear);
    API_METHOD_WRAPPER_1(DspNetwork, get);
    API_METHOD_WRAPPER_1(DspNetwork, createTest);
    API_VOID_METHOD_WRAPPER_1(DspNetwork, setForwardControlsToParameters);
    API_METHOD_WRAPPER_1(DspNetwork, setParameterDataFromJSON);
    API_METHOD_WRAPPER_3(DspNetwork, createAndAdd);
    API_METHOD_WRAPPER_2(DspNetwork, createFromJSON);
    API_METHOD_WRAPPER_0(DspNetwork, undo);
};
```

No `deleteIfUnused` in the wrapper -- this method IS registered via ADD_API_METHOD but has no wrapper entry. Wait -- actually checking again, `deleteIfUnused` is NOT in the ADD_API_METHOD list in the constructor (lines 163-175). Looking at the base JSON, `deleteIfUnused` IS listed as a method. Let me verify... The Doxygen comment exists on line 554 but no ADD_API_METHOD call. This means `deleteIfUnused` is documented but may not be exposed to scripts. Actually, re-reading: it IS in the Wrapper? No, it's not there either. This is a method that exists as public C++ but is called internally (from `clear()` at line 930). It appears in the base JSON because it has a Doxygen comment, but it is NOT registered as a script API method.

**Correction:** Looking more carefully at the base JSON, `deleteIfUnused` has a Doxygen-style comment in the header (line 554) and appears in the generated JSON. However, it is NOT in the ADD_API_METHOD list, meaning it is exposed via the API generator's Doxygen parsing but may not actually be callable from script. The Step B agent should verify whether this method is actually accessible.

## Constants

Zero constants. The constructor passes `numConstants=2` to `ConstScriptingObject` but makes no `addConstant()` calls. The slots appear unused.

## Inner Classes and Key Infrastructure

### DspNetwork::Holder (lines 196-267)

The hosting interface that a `ProcessorWithScriptingContent` must implement to own DspNetwork instances. Extends `RuntimeTargetHolder`.

Key members:
- `ReferenceCountedArray<DspNetwork> networks` -- all networks owned by this processor
- `WeakReference<DspNetwork> activeNetwork` -- the currently processing network
- `WeakReference<DspNetwork> debuggedNetwork` -- for debug/swap pattern
- `dll::ProjectDll::Ptr projectDll` -- reference to compiled C++ project DLL
- `SimpleReadWriteLock connectLock` -- network-level lock
- `ExternalDataHolder* dataHolder` -- external data (tables, sliders, audio files)
- `WeakReference<snex::Types::VoiceResetter> vk` -- voice killer for polyphonic

Key methods:
- `getOrCreate(String id)` -- creates or retrieves a DspNetwork by ID; in backend, loads from XML file in Networks folder; creates default `container.chain` root if no file exists
- `getOrCreate(ValueTree v)` -- creates from explicit ValueTree
- `setActiveNetwork(n)` -- sets active + updates processor parameter slots
- `toggleDebug()` -- swaps activeNetwork and debuggedNetwork
- `saveNetworks(ValueTree&)` -- serializes all networks; in backend, optionally embeds SNEX/Faust source or saves to external XML files
- `restoreNetworks(ValueTree&)` -- deserializes; loads embedded network data or from file handler
- `clearAllNetworks()` -- write-locks and clears
- `initialiseProjectDll(mc)` -- loads the compiled project DLL (backend only)

### DspNetwork::FaustManager (lines 275-337)

Manages Faust DSP code editing and compilation within the network. One instance per DspNetwork (member `faustManager`).

Listener interface `FaustListener`:
- `faustFileSelected(File)` -- synchronous, when a faust file is selected for editing
- `preCompileFaustCode(File)` -- synchronous, before compilation
- `compileFaustCode(File) -> Result` -- on sample loading thread
- `faustCodeCompiled(File, Result)` -- on message thread, after compilation

Compilation uses `killVoicesAndCall` to safely run on the sample loading thread.

### DspNetwork::CodeManager (lines 340-451, guarded by `HISE_INCLUDE_SNEX`)

Manages SNEX (C++ JIT) source code for custom nodes. Contains:
- `SnexSourceCompileHandler` -- compile handler that runs compilation on a background thread, with `SimpleReadWriteLock compileLock` for synchronization
- `Entry` -- pairs a SNEX workbench with parameter metadata (ValueTree XML)
- Code is stored in `DspNetworks/CodeLibrary/{typeId}/{classId}.h`

### DspNetwork::NetworkParameterHandler (lines 622-656)

Bridges the network's root node parameters to the `ScriptParameterHandler` interface. This is how the host (DAW) sees the network's parameters.

Delegates all operations to `root->getParameterFromIndex(i)`:
- `getNumParameters()` -> root's parameter count
- `getParameter(index)` -> root parameter value
- `setParameter(index, value)` -> `setValueAsync` on root parameter

### DspNetwork::VoiceSetter / NoVoiceSetter (lines 163-183)

RAII wrappers for setting the current voice index on the PolyHandler:
- `VoiceSetter(DspNetwork&, int voiceIndex)` -- scoped voice activation
- `NoVoiceSetter(DspNetwork&)` -- scoped "all voices" mode

### DspNetwork::SelectionListener (lines 593-598)

Interface for UI components to be notified of node selection changes. The `SelectionUpdater` inner class bridges from the `SelectedItemSet<NodeBase::Ptr>` change broadcaster.

### DspNetwork::AnonymousNodeCloner (lines 151-161)

RAII helper that temporarily redirects node creation to a different NodeBase::Holder (for creating isolated node copies without polluting the main network's node list).

### DspNetwork::DynamicParameterModulationProperties (lines 740-769)

Manages the modulation properties for the network's root parameters. Listens to ValueTree changes for:
- `ExternalModulation`, `ID`, `ModColour` property changes
- `ModulationBlockSize` changes (triggers re-prepare)
- Connection child additions/removals

Communicates with `ExtraModulatorRuntimeTargetSource` to update modulation chain metadata when parameters or connections change.

### DspNetwork::IdChange (lines 185-194)

Simple struct holding `oldId` and `newId` strings, used when cloning ValueTrees with new IDs to track renaming for connection updates.

## ValueTree Architecture

The network is backed by a `ValueTree data` of type `PropertyIds::Network`. Structure:

```
Network [ID, AllowCompilation, AllowPolyphonic, CompileChannelAmount, 
         ModulationBlockSize, HasTail, SuspendOnSilence, Version]
  +-- Node [ID, FactoryPath]  (root node, typically "container.chain")
        +-- Nodes
        |     +-- Node [ID, FactoryPath, ...]
        |     +-- ...
        +-- Parameters
              +-- Parameter [ID, Value, ...]
                    +-- Connections
                          +-- Connection [NodeId, ...]
```

Key ValueTree properties on the Network level:
- `AllowCompilation` (bool, default false) -- whether the network can be compiled to C++
- `AllowPolyphonic` (bool, default=isPoly) -- polyphonic processing capability
- `CompileChannelAmount` (int, default 2) -- channel count for compilation
- `ModulationBlockSize` (int, default 0) -- modulation update rate (0 = use audio block size)
- `HasTail` (bool, default true) -- whether the network produces tail after input stops
- `SuspendOnSilence` (bool, default false) -- whether to suspend processing on silence
- `Version` (string, default "0.0.0") -- network version

## Processing Pipeline

### prepareToPlay (lines 745-781)

1. Runs pending post-init functions
2. Acquires write lock on connection lock (if already initialized)
3. Adjusts block size via `dynamicParameterProperties.data.getBlockSize(blockSize)`
4. Stores `originalSampleRate` and `currentSpecs`
5. Calls `getRootNode()->prepare(currentSpecs)` with voiceIndex from polyHandler
6. Runs post-init again, then `getRootNode()->reset()`
7. Sets `initialised = true`

### process (lines 704-716)

```cpp
void DspNetwork::process(ProcessDataDyn& data)
{
    TRACE_DSP();
    if(!isInitialised()) return;
    if (auto s = SimpleReadWriteLock::ScopedTryReadLock(getConnectionLock()))
    {
        if (exceptionHandler.isOk())
            getRootNode()->process(data);
    }
}
```

Key safety patterns:
- **Not-initialized guard** -- returns silently before first prepare
- **TryReadLock** -- non-blocking on audio thread; if write lock held (network being modified), processing is skipped (no audio, not blocked)
- **Exception check** -- if any node has a pending error, processing is halted

### processBlock (script API, lines 783-811)

Converts a script `var` (array of Buffer objects) into `ProcessDataDyn` and calls `process()`. Validates that all buffers have matching sample counts.

## Undo System

- `UndoManager um` -- JUCE UndoManager, private member
- **Backend default:** `enableUndo = true` (line 800)
- **Frontend default:** `enableUndo = false` (line 803)
- Timer-based transaction grouping: when enabled, timer fires every 1500ms calling `um.beginNewTransaction()`
- `getUndoManager(returnIfPending)` -- returns nullptr if undo is disabled or if currently performing undo/redo (unless `returnIfPending` is true)
- `undo()` script method calls `getUndoManager(true)->undo()`
- `setEnableUndoManager(bool)` -- can enable/disable at runtime; starts/stops timer accordingly

## Node Creation Flow

### create(path, id) (lines 814-838)

1. Calls `checkValid()` (ensures parent holder is alive)
2. Checks if node with `id` already exists via `get(id)` -- returns existing if found
3. If `id` is empty, generates unique name from path suffix
4. Creates ValueTree `{Node, ID=id, FactoryPath=path}`
5. Calls `createFromValueTree(isPoly, newNodeData)` which iterates all registered factories

### createFromValueTree (lines 1040-1117)

The core node instantiation method:
1. If not polyphonic, forces `createPolyIfAvailable = false`
2. Checks for existing node (unless `forceCreate`)
3. Recursively creates child nodes first
4. Iterates `nodeFactories` -- each factory's `createNode()` checks if the path prefix matches its ID
5. If a factory creates the node, prepares it with current specs if sample rate is set
6. Assigns unique ID and adds to node list

### createAndAdd (lines 840-856)

Convenience: calls `create()` then `n->setParent(parent, -1)` to add to a container.

### createFromJSON (lines 858-887)

Recursively creates nodes from a JSON structure:
- Reads `FactoryPath` and `ID` from each object
- Calls `createAndAdd` for each
- Recursively processes `Nodes` array for child nodes

## AssignableObject (bracket access)

`DspNetwork` supports `network["nodeId"]` syntax:
- `getCachedIndex(var)` -- if string, searches nodes by ID; if int, uses directly
- `getAssignedValue(int)` -- returns `nodes[index]`
- `assign(int, var)` -- reports script error ("Can't assign to this expression")

## ForwardControls Pattern

The `forwardControls` flag (default `true`) determines whether UI control values are forwarded to network parameters or to regular script callbacks.

- `setForwardControlsToParameters(bool)` -- sets the flag
- `isForwardingControlsToParameters()` -- reads the flag
- `Holder::getCurrentNetworkParameterHandler()` -- if forwarding, returns `networkParameterHandler`; otherwise returns the content's parameter handler

This is how scriptnode networks integrate with the HISE parameter system: when forwarding is enabled, UI knobs drive network parameters directly.

## Backend vs Frontend Differences

| Feature | Backend | Frontend |
|---------|---------|----------|
| Undo default | enabled | disabled |
| Host factory | `dll::BackendHostFactory` (loads project DLL) | `hise::FrontendHostFactory` (compiled-in) |
| Template factory | `TemplateNodeFactory` added | not added |
| `createTest()` | returns `ScriptNetworkTest` | returns `var()` (no-op) |
| `createAllNodesOnce()` | full node database generation | N/A |
| `checkIfDeprecated()` | checks all ValueTree nodes | no-op |
| `checkBeforeCompilation()` | validates wrappable nodes | no-op |
| Network save | to XML files or embedded | N/A |

## Threading and Locking

- **`getConnectionLock()`** returns `parentHolder->getNetworkLock()` -- a `SimpleReadWriteLock`
- **Audio thread** uses `ScopedTryReadLock` in `process()` -- never blocks
- **Modification operations** (node creation, deletion, connection changes) acquire write lock
- **`prepareToPlay`** acquires write lock if already initialized
- **Selection changes** dispatch asynchronously via message thread
- **Faust compilation** runs on sample loading thread via `killVoicesAndCall`

## Error/Exception Handling

`ScriptnodeExceptionHandler` (member `exceptionHandler`):
- Maintains a list of `{node, error}` items
- Processing halts if any error exists (`isOk()` returns false)
- Error codes include: ChannelMismatch, BlockSizeMismatch, IllegalFrameCall, SampleRateMismatch, TooManyChildNodes, TooManyParameters, NoMatchingParent, DeprecatedNode, IllegalPolyphony, etc.
- Some errors are auto-fixable: `IllegalCompilation`/`IllegalNoCompilation` toggle the AllowCompilation flag; `NoMatchingParent` wraps into midi chain
- `errorBroadcaster` notifies listeners of error changes

## Deprecation Checking

`DeprecationChecker` (DspNetwork.h:69-90, DspNetwork.cpp:2238-2312):
- Checks connections/modulation targets for deprecated properties
- `OpTypeNonSet` -- connection OpType should be "SetValue" (use control.pma instead)
- `ConverterNotIdentity` -- connection Converter should be "Identity" (use control.xfader instead)
- Also removes obsolete properties: `LockNumChannels`, `CommentWidth`, `Public`, `BypassRampTimeMs`

## Related Infrastructure

### NodeFactory (StaticNodeWrappers.h:688+)

Base class for all node factories. Each factory:
- Has an `Identifier` ID (e.g., "container", "core", "math")
- Maintains `monoNodes` and `polyNodes` item lists
- `createNode(ValueTree, bool createPolyIfAvailable)` -- matches path prefix to factory ID, then looks up the node creator callback
- Registration methods: `registerNode<T>()`, `registerPolyNodeRaw<Mono, Poly>()`, `registerNodeRaw<T>()`, `registerNodeWithLambda(id, callback)`

### ScriptNetworkTest (TestClasses.h:110+)

Backend-only test harness created by `createTest(testData)`:
- Wraps a workbench-based test runner
- Methods: `runTest()`, `setTestProperty()`, `setProcessSpecs()`, `expectEquals()`, `setWaitingTime()`, `addRuntimeFunction()`
- Test data includes NodeId property set to network's ID
- Uses `ScriptnodeCompileHandlerBase` for prepare/process

### DspNetworkListeners (DspNetwork.h:898+, backend only)

Listener infrastructure for the backend IDE:
- `Base` -- ValueTree any-change listener with debouncing (ignores first 2 seconds after creation)
- `PatchAutosaver` -- auto-saves network to XML on changes, with ValueTree stripping
- `LambdaAtNetworkChange` -- synchronous change notification
- `MacroParameterDragListener` -- mouse listener for parameter drag connections in the graph editor

### HostHelpers (DspNetwork.h:869+)

Static helpers for the host/node interface:
- `getNumMaxDataObjects(ValueTree, DataType)` -- counts max data object indices in a network tree
- `setNumDataObjectsFromValueTree(OpaqueNode, ValueTree)` -- configures opaque node data counts

## Key Private Members

```cpp
ValueTree data;                          // The network ValueTree
const bool isPoly;                       // Polyphonic flag (set at construction)
bool forwardControls = true;             // Forward UI controls to parameters
bool enableUndo;                         // Backend: true, Frontend: false
UndoManager um;                          // JUCE UndoManager
PrepareSpecs currentSpecs;               // Current audio specs
double originalSampleRate = 0.0;         // Pre-modulation-block-size sample rate
snex::Types::PolyHandler polyHandler;    // Voice management
snex::Types::DllBoundaryTempoSyncer tempoSyncer;  // Tempo sync
ModValue networkModValue;                // Network-level modulation output
ScriptnodeExceptionHandler exceptionHandler;
OwnedArray<NodeFactory> ownedFactories;  // Owned factory instances
Array<WeakReference<NodeFactory>> nodeFactories;  // All factories (owned + external)
WeakReference<ExternalDataHolder> dataHolder;
WeakReference<Holder> parentHolder;
var localCableManager;                   // routing::local_cable_base::Manager
SelectedItemSet<NodeBase::Ptr> selection; // Node selection for IDE
bool enableCpuProfiling = false;
float* currentData[NUM_MAX_CHANNELS];    // Temp buffer pointers for processBlock
String initialId;                        // ID at construction for mismatch detection
```

## obtainedVia

`Engine.createDspNetwork(id)` -- requires the script processor to implement `DspNetwork::Holder`:

```cpp
var ScriptingApi::Engine::createDspNetwork(String id)
{
    if (auto holder = dynamic_cast<scriptnode::DspNetwork::Holder*>(getScriptProcessor()))
        return holder->getOrCreate(id);
    reportScriptError("Not available on this script processor");
}
```

The Holder::getOrCreate pattern:
1. Checks existing networks by ID
2. In backend: searches for matching XML file in DspNetworks folder
3. Creates new network with `container.chain` root if no file found
4. Sets as active network
