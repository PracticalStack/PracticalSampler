# OpaqueNode -- Type-Erased Node Runtime Infrastructure

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/node_api/nodes/OpaqueNode.h`
- `hi_dsp_library/node_api/nodes/OpaqueNode.cpp`
- `hi_dsp_library/node_api/nodes/prototypes.h`
- `hi_tools/hi_tools/CustomDataContainers.h` (ObjectStorage)

Related infrastructure: `core.md` (sections 6 and 8 for mothernode and parameters)

---

## 1. Purpose

OpaqueNode is the central type-erasure mechanism in scriptnode. Every node --
regardless of its C++ type -- is stored at runtime as an OpaqueNode. This
avoids template explosion: instead of the runtime needing to know about
hundreds of concrete node types, it works with a single struct that holds
C-style function pointers for all node callbacks.

OpaqueNode serves three runtime contexts:

1. **Interpreted mode (HISE IDE):** Nodes are instantiated via factory lookup,
   with each node's callbacks dispatched through OpaqueNode's function pointer
   table. This is the normal development workflow.

2. **DLL mode (Project DLL):** Custom C++ nodes compiled into a shared library.
   The DLL exports C functions that the host calls to populate OpaqueNode
   instances. The function pointer table crosses the DLL boundary.

3. **Compiled plugin (C++ export):** When a scriptnode network is exported to
   C++, nodes are compiled directly into the binary. OpaqueNode still exists
   as the runtime wrapper, but the function pointers resolve to direct calls
   in the same binary -- no DLL indirection. The `StaticLibraryHostFactory`
   handles this case.

---

## 2. OpaqueNode Structure

### Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `NumMaxParameters` | 16 | Maximum parameters per node |
| `SmallObjectSize` | 128 | Inline buffer threshold in bytes |

### Type aliases

- `MonoFrame` = `span<float, 1>` -- single-channel frame
- `StereoFrame` = `span<float, 2>` -- two-channel frame

### Function pointer table

These are the C-style function pointers that dispatch to the concrete node:

| Field | Prototype | Dispatches to |
|-------|-----------|---------------|
| `processFunc` | `void(*)(void*, ProcessDataDyn*)` | `T::process(ProcessDataDyn&)` |
| `monoFrame` | `void(*)(void*, MonoFrame*)` | `T::processFrame(span<float,1>&)` |
| `stereoFrame` | `void(*)(void*, StereoFrame*)` | `T::processFrame(span<float,2>&)` |
| `prepareFunc` | `void(*)(void*, PrepareSpecs*)` | `T::prepare(PrepareSpecs)` |
| `resetFunc` | `void(*)(void*)` | `T::reset()` |
| `eventFunc` | `void(*)(void*, HiseEvent*)` | `T::handleHiseEvent(HiseEvent&)` |
| `externalDataFunc` | `void(*)(void*, const ExternalData*, int)` | `T::setExternalData(ExternalData&, int)` |
| `modFunc` | `int(*)(void*, double*)` | `T::handleModulation(double&)` |
| `initFunc` | `void(*)(void*, ObjectWithValueTree*)` | `T::initialise(ObjectWithValueTree*)` |
| `destructFunc` | `void(*)(void*)` | `T::~T()` |
| `connectRuntimeFunc` | `void(*)(void*, bool, const connection&)` | `T::connectToRuntimeTarget(bool, connection&)` |

All function pointers take `void*` as the first argument -- the raw pointer
to the type-erased node object stored in the ObjectStorage buffer.

### Metadata fields

| Field | Type | Default | Meaning |
|-------|------|---------|---------|
| `numParameters` | int | 0 | Number of registered parameters |
| `numChannels` | int | -1 | Fixed channel count (-1 = dynamic) |
| `numDataObjects[5]` | int[] | {0} | Count per ExternalData::DataType |
| `isPoly` | bool | false | Node supports polyphonic voices |
| `isPolyPossible` | bool | false | Polyphonic variant exists in factory |
| `shouldProcessHiseEvent` | bool | false | Node receives MIDI events |
| `isNormalised` | bool | false | Modulation output is 0..1 |
| `hasTail_` | bool | true | Produces output after input stops |
| `canBeSuspended_` | bool | false | Can be suspended on silence |
| `mnPtr` | mothernode* | nullptr | Pointer to mothernode base (if applicable) |
| `description` | String | "" | Human-readable description (debug only) |

### Non-copyable, non-movable

OpaqueNode deletes both copy and move constructors. Nodes are always
created in-place and never relocated. This is critical because the
function pointers and internal object storage depend on stable addresses.

---

## 3. The create<T>() Mechanism

The templated `create<T>()` method is the core setup path. It transforms a
concrete C++ node type T into a type-erased OpaqueNode. The sequence:

### Step 1: Teardown previous node
```cpp
callDestructor();  // calls destructFunc on existing object, frees storage
```

### Step 2: Allocate storage
```cpp
allocateObjectSize(sizeof(T));  // delegates to ObjectStorage::setSize()
```

### Step 3: Wire function pointers
All pointers are set via `prototypes::static_wrappers<T>`:
```cpp
destructFunc = prototypes::static_wrappers<T>::destruct;
prepareFunc  = prototypes::static_wrappers<T>::prepare;
resetFunc    = prototypes::static_wrappers<T>::reset;
processFunc  = prototypes::static_wrappers<T>::process<ProcessDataDyn>;
monoFrame    = prototypes::static_wrappers<T>::processFrame<MonoFrame>;
stereoFrame  = prototypes::static_wrappers<T>::processFrame<StereoFrame>;
initFunc     = prototypes::static_wrappers<T>::initialise;
eventFunc    = prototypes::static_wrappers<T>::handleHiseEvent;
```

### Step 4: Placement-new the object
```cpp
auto t = prototypes::static_wrappers<T>::create(getObjectPtr());
// This calls: new (obj) T();
```

### Step 5: Query compile-time traits
Uses `if constexpr` with `prototypes::check::*` SFINAE traits to detect
whether T provides optional callbacks and properties:

- `setExternalData` -- wired if present, else `prototypes::noop::setExternalData`
- `handleModulation` -- wired if present, else `prototypes::noop::handleModulation`
- `connectToRuntimeTarget` -- wired if present
- `isProcessingHiseEvent` -- queries runtime value
- `hasTail` -- queries runtime value
- `isSuspendedOnSilence` -- queries runtime value
- `getFixChannelAmount` -- queries static value from ObjectType
- `isNormalisedModulation` -- queries static value from WrappedObjectType
- `isBaseOf<mothernode>()` -- extracts mothernode pointer through wrapper layers

### Step 6: Create and store parameters
```cpp
ParameterDataList pList;
t->createParameters(pList);
fillParameterList(pList);  // sorts by index, stores in parameters array
```

Parameters are sorted by their `info.index` field and stored in a JUCE Array.
The `getParameter(int)` method provides indexed access (bounded).

---

## 4. The prototypes Namespace

Located in `prototypes.h`, this namespace defines the type-erasure bridge.

### Function pointer typedefs

Plain C function pointer types for each callback:
```cpp
typedef void(*prepare)(void*, PrepareSpecs*);
typedef void(*reset)(void*);
typedef void(*handleHiseEvent)(void*, HiseEvent*);
typedef void(*destruct)(void*);
// ... etc.
```

Note: All use pointer-to-struct arguments (e.g., `PrepareSpecs*` not
`PrepareSpecs`) because C function pointers cannot take C++ references.

### static_wrappers<T>

Template struct that generates static wrapper functions for any node type T.
Each wrapper casts `void*` to `T*` and calls the member function:

```cpp
template <typename T> struct static_wrappers
{
    static T* create(void* obj) { return new (obj) T(); }
    static void destruct(void* obj) { static_cast<T*>(obj)->~T(); }
    static void prepare(void* obj, PrepareSpecs* ps)
    {
        static_cast<T*>(obj)->prepare(*ps);
    }
    static void process(void* obj, ProcessDataType* data)
    {
        static_cast<T*>(obj)->process(*data);
    }
    // ... same pattern for all callbacks
};
```

The `prepare` wrapper has special error handling: when `THROW_SCRIPTNODE_ERRORS`
is not defined (i.e., inside a DLL), it wraps the call in a try/catch that
stores the error in `DynamicLibraryErrorStorage::currentError`. This prevents
C++ exceptions from crossing the DLL boundary.

### check namespace (SFINAE trait detectors)

The `prototypes::check` namespace contains ~18 SFINAE detectors, each using
the classic sizeof-based technique:

```cpp
template <typename T> class setExternalData
{
    typedef char one; struct two { char x[2]; };
    template <typename C> static one test(decltype(&C::setExternalData));
    template <typename C> static two test(...);
public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};
```

These detect whether a type has a particular method. Used by `create<T>()`
to conditionally wire function pointers only for methods the node implements.

Detected traits:
`setExternalData`, `initialise`, `processSample`, `isPolyphonic`,
`isSuspendedOnSilence`, `hasTail`, `createParameters`, `prepare`, `process`,
`processFrame`, `reset`, `connectToRuntimeTarget`, `getDescription`,
`getDefaultValue`, `isNormalisedModulation`, `handleModulation`,
`getFixChannelAmount`, `isProcessingHiseEvent`, `createExternalModulationInfo`

### noop namespace

Provides do-nothing implementations for optional callbacks. When a node does
not implement `setExternalData` or `handleModulation`, OpaqueNode wires the
noop version instead of leaving a nullptr.

---

## 5. Memory Management: ObjectStorage

OpaqueNode stores the type-erased node in an `ObjectStorage<128, 16>` member.
This is a small-buffer-optimized allocator:

### Allocation strategy

- **Small objects (<= 128 bytes):** Stored in an inline stack buffer within
  the OpaqueNode struct itself. No heap allocation.
- **Large objects (> 128 bytes):** Allocated on the heap via `HeapBlock<uint8>`.

The 128-byte threshold covers simple nodes (a few floats, a parameter or two).
Complex nodes with multiple PolyData members, large state, or embedded
containers will exceed this and go to the heap.

### Alignment

All pointers are 16-byte aligned (the `Alignment` template parameter).
The `alignPtr()` method adjusts within the buffer to ensure the returned
object pointer meets this alignment. This is important for SIMD operations
that some nodes perform on their internal state.

### Debug guards

In debug builds, ObjectStorage adds 16-byte guard patterns (0xCD) before
and after the allocated region. `checkGuards()` is called on every operation
to detect buffer overruns in node implementations.

### Lifecycle

1. `allocateObjectSize(sizeof(T))` -- called by `create<T>()`, delegates
   to `ObjectStorage::setSize()`
2. Placement-new constructs T in the allocated buffer
3. Throughout the node's lifetime, `getObjectPtr()` returns the aligned pointer
4. `callDestructor()` calls `destructFunc(ptr)` (which calls `T::~T()`),
   then `object.free()` releases the storage
5. OpaqueNode's own destructor calls `callDestructor()`

### Non-relocatable

ObjectStorage supports `swapWith()` but OpaqueNode is non-copyable and
non-movable. Once created, a node's memory location is fixed.

---

## 6. Factory System: dll::FactoryBase

The factory system maps node IDs (strings) to their instantiation functions.
All factories share the `FactoryBase` interface:

```cpp
struct FactoryBase
{
    virtual int getNumNodes() const = 0;
    virtual String getId(int index) const = 0;
    virtual bool initOpaqueNode(OpaqueNode* n, int index, bool polyphonicIfPossible) = 0;
    virtual int getNumDataObjects(int index, int dataTypeAsInt) const = 0;
    virtual int getWrapperType(int index) const = 0;
    virtual int getHash(int index) const = 0;
    virtual bool isThirdPartyNode(int index) const = 0;
    virtual void deinitOpaqueNode(OpaqueNode* n) {}
};
```

### StaticLibraryHostFactory

Used for compiled plugins and embedded nodes. Stores an array of `Item`
structs, each containing:
- `id` -- node ID string
- `f` -- lambda that calls `n->create<MonoType>()`
- `pf` -- lambda that calls `n->create<PolyType>()` (nullptr if mono-only)
- `numDataObjects[5]` -- external data slot counts per type
- `isModNode` -- whether this is a modulation node
- `networkData` -- serialized network data for interpreted networks

Registration happens at static init time:
```cpp
factory.registerNode<wrap::node<my_node>>();           // mono only
factory.registerPolyNode<wrap::node<my_node<1>>,
                         wrap::node<my_node<NV>>>();   // mono + poly
factory.registerDataNode<MyNetworkData>();             // interpreted network
```

`initOpaqueNode()` selects the mono or poly lambda based on the
`polyphonicIfPossible` flag and whether a poly function exists.

### DynamicLibraryHostFactory

Wraps a `ProjectDll::Ptr` and delegates all calls to the DLL. Lightweight
(just a reference-counted pointer). Created when the HISE IDE loads a
project DLL.

### ProjectDll

Loads a `.dll`/`.dylib`/`.so` and resolves a fixed set of C functions:

| Exported function | C signature |
|-------------------|-------------|
| `getNumNodes` | `int()` |
| `getNodeId` | `size_t(int, char*)` |
| `initOpaqueNode` | `void(OpaqueNode*, int, bool)` |
| `deInitOpaqueNode` | `void(OpaqueNode*)` |
| `getNumDataObjects` | `int(int, int)` |
| `getWrapperType` | `int(int)` |
| `getHash` | `int(int)` |
| `getError` | `Error()` |
| `clearError` | `void()` |
| `isThirdPartyNode` | `bool(int)` |
| `getDllVersionCounter` | `int()` |

The `DllUpdateCounter` (currently 8) is checked at load time. If the DLL
was compiled against a different API version, the load fails with a version
mismatch error. This prevents crashes from ABI incompatibilities.

---

## 7. Dispatch Flow: Interpreted vs. Compiled

### Interpreted mode (HISE IDE with DLL or builtin nodes)

```
UI/ValueTree change
  -> parameter::data::callback (function pointer)
    -> concrete T::setParameterStatic<P>(void*, double)
      -> modifies T's internal state

Audio thread:
  Container::process()
    -> OpaqueNode::process(ProcessDataDyn&)
      -> processFunc(getObjectPtr(), &data)
        -> static_wrappers<T>::process(void*, ProcessDataDyn*)
          -> static_cast<T*>(obj)->process(data)
```

Each call goes through one level of function pointer indirection. The cost
is a single indirect call per callback per node per audio block.

### Compiled plugin (C++ export)

The same `OpaqueNode::process()` call chain exists, but since the
`StaticLibraryHostFactory` registered the function pointers from the same
binary, the compiler/linker can potentially devirtualize. In practice, the
function pointer indirection remains, but:

- No DLL boundary crossing
- No exception catching overhead (THROW_SCRIPTNODE_ERRORS is defined)
- All code is in the same translation unit (unity build), enabling
  link-time optimization (LTO) to inline through the pointer

### prepare() calls reset()

An important behavioral detail: `OpaqueNode::prepare()` automatically calls
`resetFunc()` after `prepareFunc()`:

```cpp
void OpaqueNode::prepare(PrepareSpecs ps)
{
    if (prepareFunc)
    {
        prepareFunc(getObjectPtr(), &ps);
        resetFunc(getObjectPtr());
    }
}
```

This ensures nodes are in a clean state after preparation. Individual nodes
do not need to call reset() from within their prepare() implementation.

---

## 8. Parameter Handling Through OpaqueNode

Parameters are stored as a JUCE `Array<parameter::data>` inside OpaqueNode.
During `create<T>()`, the node's `createParameters()` populates a
`ParameterDataList` which is then sorted by index and stored.

### Parameter access

- `getParameter(int index)` -- returns pointer to `parameter::data` (bounded)
- `ParameterIterator` -- range-based iteration over all parameters
- `createParameters(ParameterDataList&)` -- copies parameters out with cloned
  callback connections (used when rebuilding the parameter tree)

### Parameter dispatch

Each `parameter::data` holds a `parameter::dynamic` callback -- a
`void(*)(void*, double)` function pointer plus a `void*` object pointer.
When the UI or modulation system changes a parameter value, it calls
this function pointer directly, bypassing OpaqueNode entirely.

The OpaqueNode's own `setParameterStatic<P>()` is a stub that asserts
false -- it should never be called because parameters are always forwarded
directly to the inner node's callback.

---

## 9. External Data Initialization

`OpaqueNode::initExternalData()` iterates all ExternalData types and
forwards each data object from the ExternalDataHolder to the node:

```cpp
void initExternalData(ExternalDataHolder* holder)
{
    int totalIndex = 0;
    auto initAll = [&](ExternalData::DataType d)
    {
        for (int i = 0; i < holder->getNumDataObjects(d); i++)
            setExternalData(holder->getData(d, i), totalIndex++);
    };
    ExternalData::forEachType(initAll);
}
```

The `totalIndex` counter is global across all data types. This means if a
node has 2 tables and 1 slider pack, the slider pack gets index 2 (not 0).
The ordering follows `ExternalData::DataType` enum order:
Table(0), SliderPack(1), AudioFile(2), FilterCoefficients(3), DisplayBuffer(4).

---

## 10. Error Handling Across DLL Boundary

Inside a DLL (`THROW_SCRIPTNODE_ERRORS` not defined), the `prepare()` wrapper
catches `scriptnode::Error` exceptions and stores them in thread-local-like
`DynamicLibraryErrorStorage::currentError`. The host then retrieves errors
via the `getError()` / `clearError()` DLL functions.

This is necessary because C++ exceptions cannot safely cross DLL boundaries
(different runtime libraries, different exception handling implementations).

---

## 11. InterpretedNetworkData

A small interface for networks that exist as serialized data rather than
compiled code:

```cpp
struct InterpretedNetworkData
{
    virtual String getId() const = 0;
    virtual bool isModNode() const = 0;
    virtual String getNetworkData() const = 0;
};
```

The `StaticLibraryHostFactory::registerDataNode<T>()` creates an instance
of T (which implements this interface), extracts its ID, mod status, and
serialized network data, and stores them in the factory item. When
`initOpaqueNode()` is called for such an item, both `f` and `pf` are
nullptr -- the host must handle interpreted networks differently (by
deserializing the network data into a node graph at runtime).

---

## 12. Key Design Implications for Node Documentation

### What users see vs. what happens

When a user places a node in the scriptnode editor, the runtime:
1. Looks up the node ID in the factory
2. Calls `initOpaqueNode()` which calls `OpaqueNode::create<T>()`
3. All subsequent interactions go through function pointers

Users never interact with OpaqueNode directly. But understanding this
layer explains:
- Why nodes have a fixed maximum of 16 parameters
- Why `numChannels` can be -1 (dynamic) or fixed
- Why `prepare()` always triggers `reset()`
- Why DLL nodes require recompilation when the API version changes
- Why nodes cannot be copied or moved after creation

### Performance characteristics

- Function pointer call: ~1-3ns overhead per dispatch (branch prediction
  usually hits after the first call in a block)
- Small object optimization: nodes <= 128 bytes avoid heap allocation
- Parameter callbacks bypass OpaqueNode entirely (direct function pointer)
- Frame processing has separate mono/stereo paths to avoid dynamic dispatch
  on channel count in the inner loop
