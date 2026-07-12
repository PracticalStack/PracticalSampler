# Interpreted Node Wrappers -- IDE vs Compiled Boundary

Distilled from C++ source for the scriptnode node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_scripting/scripting/scriptnode/api/StaticNodeWrappers.h`
- `hi_scripting/scripting/scriptnode/api/StaticNodeWrappers.cpp`
- `hi_scripting/scripting/scriptnode/api/ModulationSourceNode.h`
- `hi_scripting/scripting/scriptnode/node_library/HiseNodeFactory.cpp`
- `hi_scripting/scripting/scriptnode/node_library/FrontendHostFactory.cpp`
- `hi_dsp_library/node_api/nodes/OpaqueNode.h` (dll namespace)

Related infrastructure: [core.md](core.md) -- OpaqueNode, parameters, external data

---

## 1. Overview: Interpreted Mode vs Compiled Mode

Scriptnode nodes exist in the HISE IDE as C++ template classes. The "interpreted"
layer wraps these templates so they can be manipulated via the scriptnode UI --
connected to ValueTree properties, have parameters dragged, external data edited,
etc. This layer is NOT a scripting interpreter -- it is compiled C++ running with
a dynamic property/parameter binding layer on top.

In an **exported plugin**, the same C++ node code runs, but it is loaded either:
- From a **Project DLL** (compiled separately, loaded at runtime by the IDE)
- From a **StaticLibraryHostFactory** (baked directly into the plugin binary)

The key insight: **the same InterpretedNode/InterpretedModNode wrapper classes are
used in both IDE and exported plugins**. The difference is how the inner OpaqueNode
gets initialized -- via template `create<T>()` in the IDE, or via
`initFromDll(factory, index)` from a DLL/static factory.

---

## 2. Class Hierarchy

```
NodeBase                          -- base for all scriptnode nodes
  WrapperNode                     -- adds UI component creation, parameter init
    InterpretedNode               -- standard signal-processing nodes
    ModulationSourceNode          -- adds modulation output support
      InterpretedModNode          -- mod nodes (output a modulation value)
      InterpretedCableNode        -- control/cable nodes (no audio processing)
    UncompiledNode                -- placeholder when DLL is missing

InterpretedNodeBase<WType>        -- CRTP mixin (not a NodeBase)
  provides: init<T>(), initFromDll(), postInit(), reloadFromDll(),
            setOpaqueDataEditor(), external data holder management

NodeWithFactoryConnection         -- interface for DLL reload support
  reloadFromDll(FactoryBase*)
```

### Inheritance pattern

Each concrete class uses **dual inheritance**:
- `InterpretedNode` inherits from both `WrapperNode` and
  `InterpretedNodeBase<bypass::simple<OpaqueNode>>`
- `InterpretedModNode` inherits from both `ModulationSourceNode` and
  `InterpretedNodeBase<bypass::simple<wrap::mod<parameter::dynamic_base_holder, OpaqueNode>>>`
- `InterpretedCableNode` inherits from both `ModulationSourceNode` and
  `InterpretedNodeBase<OpaqueNode>` (no bypass wrapper)

The `InterpretedNodeBase` mixin is templated on the **wrapper type** -- this
determines what wrapping layers surround the inner `OpaqueNode`:
- `bypass::simple<OpaqueNode>` -- standard nodes get bypass support
- `bypass::simple<wrap::mod<..., OpaqueNode>>` -- mod nodes get bypass + mod output
- `OpaqueNode` (bare) -- cable nodes have no bypass or mod wrapper

---

## 3. Node Initialization: Two Paths

### Path A: Template initialization (IDE, built-in nodes)

Used when the HISE IDE creates a built-in node from its factory registration.

```
NodeFactory::registerNode<T, ComponentType, WrapperType>()
  -> stores WrapperType::createNode<T, ComponentType, ...> as callback
  -> when node is created from ValueTree:
     1. new InterpretedNode(network, valueTree)
     2. init<T, AddDataOffsetToUIPtr, UseNodeBaseAsUI>()
        a. obj.getWrappedObject().create<T>()   -- OpaqueNode::create wires all
           function pointers from C++ template T
        b. obj.initialise(wrapperNode)          -- connects to ValueTree
        c. postInit()                           -- creates parameters
     3. Set extraComponentFunction for UI
```

The `OpaqueNode::create<T>()` call (documented in core.md section 6) is where
the C++ template node type gets type-erased into function pointers.

### Path B: DLL/Factory initialization (exported plugin, or IDE with project DLL)

Used when loading compiled nodes from a DLL or static library.

```
initFromDll(FactoryBase* f, int index, bool addDragger)
  1. nodeFactory = f                          -- store factory reference
  2. f->initOpaqueNode(&obj, index, poly)     -- factory wires the OpaqueNode
  3. obj.initialise(wrapperNode)              -- connects to ValueTree
  4. setOpaqueDataEditor(addDragger)          -- creates external data holders
  5. postInit()                               -- creates parameters
  6. connectToRuntimeTargets()                -- wire runtime target connections
```

The factory's `initOpaqueNode()` does the same job as `OpaqueNode::create<T>()`
but without needing the template type -- the DLL provides pre-compiled init
functions.

---

## 4. The DLL Factory System

### dll::FactoryBase (abstract interface)

All node factories implement this interface:

| Method | Purpose |
|--------|---------|
| `getNumNodes()` | Number of registered node types |
| `getId(index)` | Node ID string (e.g., "core.gain") |
| `initOpaqueNode(node, index, poly)` | Wire an OpaqueNode with the node at index |
| `deinitOpaqueNode(node)` | Clean up before reload |
| `getWrapperType(index)` | 0 = normal node, 1 = mod node |
| `getNumDataObjects(index, type)` | External data slot counts |
| `getHash(index)` | Version hash for change detection |
| `isThirdPartyNode(index)` | Whether node comes from third-party code |

### dll::StaticLibraryHostFactory (exported plugins)

Used in exported plugins (`USE_FRONTEND`). Nodes are registered at compile time
via `registerNode<T>()` which stores a lambda `[](OpaqueNode* n) { n->create<T>(); }`.

Also supports `registerDataNode<T>()` for interpreted network data -- networks
that were saved as ValueTree data rather than compiled to C++.

The `isInterpretedNetwork(index)` check distinguishes compiled nodes from
embedded network data (though the comment in FrontendHostFactory.cpp says
"This functionality is removed" with a `jassertfalse`).

### dll::ProjectDll (dynamic library)

Loads a compiled `.dll`/`.dylib` at runtime. Resolves C function exports:
- `scriptnode_getNumNodes`, `scriptnode_getNodeId`, `scriptnode_initOpaqueNode`,
  `scriptnode_deInitOpaqueNode`, `scriptnode_getWrapperType`, etc.
- `DllUpdateCounter = 8` -- version check to detect stale DLLs

### dll::DynamicLibraryHostFactory

Wraps a `ProjectDll::Ptr` and implements `FactoryBase` by forwarding all calls
to the DLL's exported functions.

---

## 5. Wrapper Type Classification

The `getWrapperType(index)` return value determines which InterpretedNode class
is used to wrap the OpaqueNode:

| Value | Meaning | Wrapper Class | Registration Method |
|-------|---------|---------------|---------------------|
| 0 | Standard signal node | `InterpretedNode` | `registerNode<T>()` |
| 1 | Modulation source node | `InterpretedModNode` | `registerModNode<T>()` |

Cable/control nodes (`InterpretedCableNode`) are registered via
`registerNoProcessNode<T>()` -- these are `ModulationSourceNode` subclasses
that assert `OutsideSignalPath` and have specialized parameter holder logic.

The wrapper type is stored in `StaticLibraryHostFactory::Item::isModNode` and
queried by both `HiseNodeFactory` and `FrontendHostFactory` to decide which
concrete wrapper to instantiate.

---

## 6. Parameter Connection: ValueTree to C++

### During initialization (postInit)

```cpp
void InterpretedNodeBase::postInit()
{
    ParameterDataList pData;
    obj.getWrappedObject().createParameters(pData);  // C++ node fills parameter list
    asWrapperNode()->initParameterData(pData);        // WrapperNode creates Parameter objects
}
```

`createParameters()` is the C++ node's method that registers its parameters with
names, ranges, and callback function pointers. `initParameterData()` creates
`Parameter` objects that bridge the ValueTree property system to these callbacks.

### Runtime parameter flow

```
UI slider change
  -> ValueTree property update
  -> Parameter::setValueAsync(double)
  -> parameter::dynamic callback (function pointer in OpaqueNode)
  -> T::setParameterStatic<P>(void*, double)
  -> actual C++ setter method
```

### Bypass parameter

Bypass is handled as a special parameter on the wrapper type:
```cpp
void InterpretedNode::setBypassed(bool shouldBeBypassed)
{
    WrapperNode::setBypassed(shouldBeBypassed);
    WrapperType::setParameter<bypass::ParameterId>(&this->obj, (double)shouldBeBypassed);
}
```

This routes through the `bypass::simple` wrapper layer.

---

## 7. External Data Connection

### OpaqueNodeDataHolder

When a node uses external data (tables, slider packs, audio files, filters,
display buffers), the `InterpretedNodeBase::setOpaqueDataEditor()` method
creates an `OpaqueNodeDataHolder`.

The holder:
1. Queries `OpaqueNode::numDataObjects[]` for counts of each data type
2. Creates `data::dynamic::*` objects for each slot (table, sliderpack,
   audiofile, filter, displaybuffer)
3. Initializes each data object with the parent node's ValueTree
4. Calls `OpaqueNode::setExternalData()` to wire the data into the C++ node
5. Creates UI editors via `OpaqueNodeDataHolder::Editor`

### Data flow

```
OpaqueNodeDataHolder
  inherits: ExternalDataHolderWithForcedUpdate (owns the data objects)
  inherits: data::base (for ExternalData pass-through)
  inherits: ForcedUpdateListener (for rebuild notifications)

setExternalData(data, index):
  1. base::setExternalData(data, index)     -- store locally
  2. opaqueNode.setExternalData(data, index) -- forward to C++ node
```

### Data type dispatch

The `create()` factory method maps `ExternalData::DataType` to concrete classes:

| DataType | Dynamic Wrapper |
|----------|----------------|
| Table | `data::dynamic::table` |
| SliderPack | `data::dynamic::sliderpack` |
| AudioFile | `data::dynamic::audiofile` |
| FilterCoefficients | `data::dynamic::filter` |
| DisplayBuffer | `data::dynamic::displaybuffer` |

### UI Editor creation

The `OpaqueNodeDataHolder::Editor` creates typed editors for each data slot.
When data count is even, it uses a two-column layout (width 440px). Otherwise
single-column. An optional modulation dragger is appended at the bottom.

---

## 8. DLL Hot-Reload (USE_BACKEND only)

When the user recompiles the project DLL while the IDE is running, nodes can
be hot-reloaded via `reloadFromDll(FactoryBase*)`:

```cpp
void InterpretedNodeBase::reloadFromDll(FactoryBase* newFactory)
{
    // 1. Disconnect runtime targets
    // 2. Deinit old OpaqueNode via old factory
    // 3. Find matching node ID in new factory
    // 4. Save all parameter values
    // 5. Remove all parameters
    // 6. initFromDll(newFactory, matchingIndex, ...)
    // 7. Restore parameter values by name+index matching
}
```

Parameter values are preserved across reloads by saving name-value pairs
and restoring by index (with a name check for safety).

---

## 9. UncompiledNode: Missing DLL Placeholder

When a node references a compiled node type but no DLL is available (or the
DLL does not contain the node), an `UncompiledNode` is created instead.

Properties:
- All processing methods are no-ops (silence)
- `getObjectPtr()` returns nullptr
- Parameters are created from the ValueTree but with dummy holders
- Shows a `ReloadComponent` UI that either:
  - Prompts "Click to compile the DLL..." (if not yet compiled)
  - Prompts "Click to reload the network..." (if DLL was hot-reloaded)
- Clicking triggers either `BackendCommandTarget::Actions::compileNetworksToDll()`
  or a full network reload from the current ValueTree

---

## 10. NodeFactory: Registration System

`NodeFactory` provides template methods that determine which wrapper class
and initialization path to use:

| Registration Method | Wrapper Class | Bypass | Mod | Signal |
|---------------------|---------------|--------|-----|--------|
| `registerNode<T>()` | InterpretedNode | Yes | No | Yes |
| `registerModNode<T>()` | InterpretedModNode | Yes | Yes | Yes |
| `registerNoProcessNode<T>()` | InterpretedCableNode | No | Cable | No |
| `registerPolyNode<M,P>()` | InterpretedNode (x2) | Yes | No | Yes |
| `registerPolyModNode<M,P>()` | InterpretedModNode (x2) | Yes | Yes | Yes |
| `registerPolyNoProcessNode<M,P>()` | InterpretedCableNode (x2) | No | Cable | No |
| `registerNodeRaw<T>()` | Custom (T::createNode) | Custom | Custom | Custom |

For polyphonic registration, both mono and poly variants are registered. The
mono variant goes into `monoNodes`, the poly into `polyNodes`. At creation time,
the network's polyphonic state determines which list is consulted.

### Template parameters on registerNode

```cpp
template <class T,
          class ComponentType = HostHelpers::NoExtraComponent,
          typename WrapperType = InterpretedNode,
          bool AddDataOffsetToUIPtr = true,
          bool UseNodeBaseAsUI = false>
void registerNode();
```

- `T` -- the C++ node type
- `ComponentType` -- UI component class (must have `createExtraComponent` static)
- `WrapperType` -- which Interpreted*Node class to use
- `AddDataOffsetToUIPtr` -- whether to compute a byte offset for the UI pointer
  (needed when node is wrapped in `wrap::data<>`)
- `UseNodeBaseAsUI` -- use the InterpretedNode itself as the UI object pointer

---

## 11. TemplateNodeFactory: Pre-built Network Templates

The `TemplateNodeFactory` provides compound node templates that are
programmatically assembled from individual nodes:

| Template | Root Container | Purpose |
|----------|---------------|---------|
| `mid_side` | container.chain | MS encode/decode with mid+side gain |
| `dry_wet` | container.split | Dry/wet mixer with crossfader |
| `feedback_delay` | container.fix32_block | Send/receive feedback loop |
| `bipolar_mod` | container.modchain | Bipolar modulation with PMA |
| `freq_split{2-5}` | container.split | Multi-band frequency splitter |
| `softbypass_switch{2-8}` | container.chain | Soft-bypassed switch matrix |

These use `TemplateNodeFactory::Builder` to construct ValueTrees programmatically,
then call `network->createFromValueTree()` to instantiate the full graph.

In `USE_BACKEND`, file-based templates from `BackendDllManager::getAllNodeTemplates()`
are also loaded and registered with lambda callbacks.

---

## 12. Runtime Target Connections

Some nodes need to connect to the host processor for runtime data (e.g., MIDI,
transport). The `runtimeTargetMaster` field handles this:

```cpp
// During init:
auto hasRuntimeTargets = cppgen::CustomNodeProperties::isRuntimeTarget<T>();
if (hasRuntimeTargets)
{
    runtimeTargetMaster = dynamic_cast<Processor*>(getScriptProcessor());
    runtimeTargetMaster->connectToRuntimeTargets(obj.getWrappedObject(), true);
}

// During destruction or disconnect:
runtimeTargetMaster->connectToRuntimeTargets(obj.getWrappedObject(), false);
```

The `connectToRuntimeTarget(bool)` virtual method is overridden by all three
wrapper classes (InterpretedNode, InterpretedModNode, InterpretedCableNode)
with identical implementations.

---

## 13. Processing: Debug Instrumentation

In the IDE, processing methods include debug instrumentation that is absent
in compiled mode:

- **NodeProfiler** -- measures processing time per node (process() only)
- **ProcessDataPeakChecker** -- validates output levels after processing
- **FrameDataPeakChecker** -- validates frame output levels
- **ExceptionHandler** -- catches `Error` exceptions during prepare()

These are all no-ops or absent in `USE_FRONTEND` builds, so exported plugins
have zero overhead from the wrapper layer.

---

## 14. Key Behavioral Differences: IDE vs Export

| Aspect | IDE (USE_BACKEND) | Export (USE_FRONTEND) |
|--------|-------------------|----------------------|
| Node creation | Template `create<T>()` | Factory `initOpaqueNode()` |
| DLL reload | Supported (hot-reload) | Not applicable |
| Uncompiled nodes | Show "click to compile" UI | Cannot occur |
| Profiling | NodeProfiler active | Compiled out |
| Peak checking | Active | Compiled out |
| Exception handling | Catches + displays errors | No try/catch overhead |
| Template nodes | Available + file templates | Not available |
| Runtime targets | Connected via Processor* | Connected via Processor* |
| Parameter binding | Identical | Identical |
| External data | Identical | Identical |
| Audio processing | Identical (same C++ code) | Identical |

The critical takeaway: **nodes behave identically in IDE and export**. The
wrapper layer adds debug instrumentation and UI binding in the IDE, but the
actual DSP code path is the same. The OpaqueNode function pointers route to
the same C++ template implementations in both cases.
