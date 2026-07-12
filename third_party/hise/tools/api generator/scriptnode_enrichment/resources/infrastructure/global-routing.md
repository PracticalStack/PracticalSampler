# Global Routing Infrastructure Reference

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.h`
- `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.cpp`
- `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingNodes.h`
- `hi_dsp_library/dsp_nodes/RoutingNodes.h` (global_cable, global_cable_cpp_manager)
- `hi_tools/hi_tools/runtime_target.h`
- `hi_dsp_library/node_api/nodes/OpaqueNode.h` (connectToRuntimeTarget)
- `hi_dsp_library/node_api/helpers/node_ids.h` (CustomNodeProperties)

---

## 1. Architecture Overview

Global routing allows communication between unrelated parts of the scriptnode
network -- or between scriptnode and the HISE module tree -- without explicit
wiring in the signal graph.

There are two distinct global routing mechanisms:

| Mechanism | SlotType | Carries | Node IDs |
|-----------|----------|---------|----------|
| **Cable** | `SlotBase::SlotType::Cable` | `double` values (0..1) and arbitrary binary data | `routing.global_cable` |
| **Signal** | `SlotBase::SlotType::Signal` | Full audio buffers (multichannel) | `routing.global_send`, `routing.global_receive` |

Both are managed by a singleton `GlobalRoutingManager` obtained via
`MainController::getGlobalRoutingManager()`. Slots are created on demand the
first time any node requests a slot ID.

---

## 2. GlobalRoutingManager

### Singleton lifecycle

```
GlobalRoutingManager::Helpers::getOrCreate(MainController* mc)
  -> mc->getGlobalRoutingManager()          // check existing
  -> if nullptr: new GlobalRoutingManager() // create
  -> mc->setGlobalRoutingManager(newP)      // store
  -> triggers RebuildModuleList
```

The manager is reference-counted (`ReferenceCountedObject`). It holds two
parallel slot lists:

- `SlotBase::List signals` -- for audio signal routing
- `SlotBase::List cables` -- for control value routing

### Slot creation and lookup

`getSlotBase(id, type)` searches the appropriate list by string ID. If no slot
exists, it creates one (Cable or Signal) and adds it. New cables also get OSC
targets attached if an OSC sender is active.

After every connection change, `removeUnconnectedSlots(type)` iterates the list
and calls `cleanup()` on each slot. Slots that return true (no remaining
targets) are removed. The `listUpdater` broadcaster notifies the UI.

### SlotBase

Base class for both Cable and Signal:

```cpp
struct SlotBase: public ReferenceCountedObject
{
    enum class SlotType { Cable, Signal, numTypes };

    const String id;           // immutable slot name
    const SlotType type;
    SimpleReadWriteLock lock;  // protects slot internals

    virtual bool cleanup() = 0;
    virtual bool isConnected() const = 0;
    virtual SelectableTargetBase::List getTargetList() const = 0;
};
```

---

## 3. Cable (Control Value Routing)

### Purpose

Routes a single `double` value (clamped to 0..1) between any number of
`CableTargetBase` endpoints. Also supports arbitrary binary data transmission
via `sendData()`.

### Key members

```cpp
struct Cable : public SlotBase, public runtime_target::source_base
{
    double lastValue = 0.0;              // last sent value
    MemoryBlock lastData;                // last sent binary data
    CableTargetBase::List targets;       // all connected targets

    ScopedPointer<RuntimeTarget> runtimeTarget;  // aggregator for compiled targets
};
```

### Value flow

1. A source calls `Cable::sendValue(source, v)`
2. The value is clamped: `lastValue = jlimit(0.0, 1.0, v)`
3. All targets except the source receive `sendValue(lastValue)`

Binary data follows the same pattern via `sendData(source, data, numBytes)`,
storing into `lastData` and forwarding to all other targets.

**Important:** The value is always clamped to 0..1. Values outside this range
are silently clamped. This is enforced in `Cable::sendValue()`.

### Target registration

`Cable::addTarget(CableTargetBase*)`:
- Acquires write lock on `SlotBase::lock`
- Adds target (deduplicating with `addIfNotAlreadyThere`)
- Immediately sends `lastValue` and `lastData` to the new target
  (so it initializes with the current state)

`Cable::removeTarget(CableTargetBase*)`:
- Acquires write lock on `SlotBase::lock`
- Removes all instances of the target

### Cleanup

`Cable::cleanup()` removes null or `shouldBeCleanedUp()` targets. Returns
true (slot can be removed) when the target list is empty.

### Runtime target bridge

The Cable also implements `runtime_target::source_base`, which allows compiled
(C++) nodes to connect via the runtime target system. The `RuntimeTarget` inner
class aggregates all compiled targets into a single `CableTargetBase`:

```
Cable (source_base)
  |-- CableTargetBase targets[]    <-- scriptnode IDE nodes (GlobalCableNode)
  |-- RuntimeTarget (CableTargetBase)
        |-- typed_target<double>[] runtimeTargets  <-- compiled global_cable nodes
```

The `connectStatic<Add>()` template handles connect/disconnect. On connect, if
the cable has pending `lastData`, it is immediately forwarded to the new
runtime target.

---

## 4. Signal (Audio Buffer Routing)

### Purpose

Routes full multichannel audio from one GlobalSendNode to one or more
GlobalReceiveNodes. Uses a shared intermediate buffer.

### Key members

```cpp
struct Signal : public SlotBase
{
    PrepareSpecs sourceSpecs;                    // specs from the send node
    span<float*, NUM_MAX_CHANNELS> channelData;  // pointers into buffer
    span<float, NUM_MAX_CHANNELS> signalPeaks;   // per-channel peak meters
    heap<float> buffer;                          // shared audio buffer
    ProcessDataDyn lastData;

    NodeBase::Ptr sendNode;          // exactly one source (or nullptr)
    NodeBase::List targetNodes;      // zero or more receivers
};
```

### Connection rules

- **One sender per signal slot.** `setSource()` fails with "Slot already has a
  send node" if a different send node tries to connect to an occupied slot.
- **Multiple receivers allowed.** `addTarget()` adds to `targetNodes`.
- **Spec matching required.** Receivers must match the sender's specs:
  - Same sample rate (Error::SampleRateMismatch)
  - Same channel count (Error::ChannelMismatch)
  - Receiver blockSize <= sender blockSize (Error::BlockSizeMismatch)

### Audio data flow

**Push (send):** `Signal::push(data, value)`
- Uses `ScopedTryReadLock` on the slot lock (non-blocking, audio-safe)
- Copies audio from ProcessDataDyn into the shared buffer with gain multiply:
  `FloatVectorOperations::copyWithMultiply(channelData[i], data[i], value, numSamples)`
- Updates `signalPeaks` for UI metering

**Pop (receive):** `Signal::pop(data, value, offset)` -> returns new offset
- Uses `ScopedTryReadLock` on the slot lock
- **Adds** (not replaces) audio from the shared buffer into the output:
  `FloatVectorOperations::addWithMultiply(data[i], channelData[i] + offset, value, numSamples)`
- Supports sub-block offset for polyphonic voice alignment
- Returns `(offset + numSamples) % sourceSpecs.blockSize`

**Key detail:** Push uses `copyWithMultiply` (overwrites buffer), pop uses
`addWithMultiply` (mixes into output). The receive node adds to its existing
signal rather than replacing it.

### Buffer allocation

`setSource()` allocates the buffer via `DspHelpers::increaseBuffer()` and sets
up channel pointers for 1-8 channels (via switch/case on numChannels). The
buffer size is `blockSize * numChannels`.

### Polyphonic offset handling

`GlobalReceiveNode` tracks a per-voice `offset` via `PolyData<int, NumVoices>`.
On note-on events, the offset is computed from the event timestamp adjusted by
the sample rate ratio between the network and the main synth chain:

```cpp
ratio = lastSpecs.sampleRate / mainSynthChain->getSampleRate();
offset = roundToInt((double)startStamp * ratio);
```

If the source and receive block sizes match, offset is forced to 0.

---

## 5. Node Implementations

### routing.global_cable (IDE node: GlobalCableNode)

- Inherits: `ModulationSourceNode`, `GlobalRoutingManager::CableTargetBase`
- SlotType: `Cable`
- Properties: `IsControlNode`, `IsFixRuntimeTarget`
- **Not in the audio signal path.** `process()` and `processFrame()` are empty.
- Acts as both a target (receives values from other cables) and a modulation
  source (sends values via parameter connections).

**Value flow:**
1. Parameter "Value" (range 0..1, default 1.0) calls `setValue()`
2. `setValue()` stores `lastValue` and calls `currentCable->sendValue(this, v)`
3. Cable broadcasts to all other targets (excluding this node)
4. When this node receives a value from another source via `sendValue(v)`:
   `getParameterHolder()->call(v)` -- forwards to connected parameter targets

**Connection lifecycle:**
- Constructor: gets/creates GlobalRoutingManager, initializes `slotId` property
- `updateConnection()`: disconnects from old cable, connects to new one
- When connecting to a cable with no existing targets, initializes the cable's
  `lastValue` with this node's `lastValue`
- Destructor: removes self from cable, cleans up unconnected slots

**Normalised range:** `isUsingNormalisedRange()` returns true -- values are
always 0..1.

### routing.global_cable (compiled node: routing::global_cable<IndexType, ParameterClass>)

- Lives in `hi_dsp_library/dsp_nodes/RoutingNodes.h`
- Inherits: `mothernode`, `control::pimpl::no_processing`,
  `control::pimpl::parameter_node_base<ParameterClass>`,
  `runtime_target::indexable_target<IndexType, RuntimeTarget::GlobalCable, double>`
- Connected to GlobalRoutingManager::Cable via the runtime_target system

**Value flow (compiled side):**
- `setValue(newValue)`: sets recursion guard, calls `sendValueToSource(v)` which
  invokes `Cable::setValueStatic()` on the manager side
- `onValue(double c)`: called when another source sends a value. If not in
  recursion, forwards to the connected parameter: `this->getParameter().call(c)`
- Recursion guard (`bool recursion`) prevents feedback loops

**Data transmission:**
- `sendData(data, numBytes)`: sends binary data to cable (with recursion guard)
- `onData(data, numBytes)`: receives binary data, deserializes from
  `MemoryInputStream` as `var`, calls `dataCallback`
- `setDataCallback(f)`: registers handler for incoming binary data

**Prepare-time validation:**
- If `IndexType::mustBeConnected()` is true and the cable is not connected,
  throws `Error::NoGlobalCable` during `prepare()`

**SN_GLOBAL_CABLE macro:**
```cpp
#define SN_GLOBAL_CABLE(hash) routing::global_cable<runtime_target::fix_hash<hash>, parameter::empty>
```
Creates a global_cable type with a fixed hash and no parameter output.

### routing.global_cable_cpp_manager

Template class for C++ nodes that need to interact with multiple global cables.
Subclass from `global_cable_cpp_manager<SN_GLOBAL_CABLE(hash1), SN_GLOBAL_CABLE(hash2), ...>`.

Key methods:
- `setGlobalCableValue<CableIndex>(value)` -- send double value to a cable
- `sendDataToGlobalCable<CableIndex>(var)` -- serialize and send data
- `registerDataCallback<CableIndex>(f)` -- register handler for incoming data
- `connectToRuntimeTarget(add, connection)` -- connects/disconnects all cables
- `prepare(ps)` -- prepares all cables, stores polyHandler
- Pending data: if `sendData` fails (cable not yet connected), data is stored
  in `pendingData` and sent on next `connectToRuntimeTarget(true, ...)`

### routing.global_send (GlobalSendNode)

- Inherits: `GlobalRoutingNodeBase`
- Properties: `UncompileableNode` (cannot be compiled to C++)
- Parameter: "Value" (range 0..1, default 1.0) -- gain multiplier for push
- `isSource()` returns true

**process():**
```cpp
if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(connectionLock))
    if (currentSlot != nullptr && !isBypassed())
        currentSlot->push(data, value);
```

**reset():** Calls `currentSlot->clearSignal()` which zeros the shared buffer.

### routing.global_receive (GlobalReceiveNode<NV>)

- Inherits: `GlobalRoutingNodeBase`
- Properties: `UncompileableNode`
- Template parameter NV for polyphonic voice count
- Parameter: "Value" (range 0..1, default 1.0) -- gain multiplier for pop
- `isSource()` returns false

**process():**
```cpp
if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(connectionLock))
    if (currentSlot != nullptr && matchesSourceSpecs(lastSpecs) && !isBypassed())
        offset = currentSlot->pop(data, value.get(), offset);
```

**handleHiseEvent():** On note-on (polyphonic only), computes starting offset
from the event timestamp adjusted by sample rate ratio.

---

## 6. Runtime Target Connection System

The `runtime_target` namespace (in `hi_tools`) provides a type-erased
connection mechanism between the GlobalRoutingManager (which lives in
`hi_scripting`) and compiled DSP nodes (which live in `hi_dsp_library`).
This bridges the DLL boundary.

### Type hierarchy

```
source_base          -- Cable implements this
  -> createConnection() -> connection struct with function pointers

target_base          -- base for anything that receives
  typed_target<T>    -- adds onValue(T), onData()
    indexable_target<IndexType, TypeIndex, T>  -- adds hash matching, connect/disconnect
```

### connection struct

A POD-like struct holding:
- `source_base* source` -- the cable
- `ConnectFunction* connectFunction` -- adds target to cable
- `ConnectFunction* disconnectFunction` -- removes target from cable
- `void* sendBackFunction` -- sends value from target back to source
- `void* sendBackDataFunction` -- sends data from target back to source

### RuntimeTarget enum

```cpp
enum class RuntimeTarget {
    Undefined, Macro, GlobalCable, NeuralNetwork,
    GlobalModulator, ExternalModulatorChain, numRuntimeTargets
};
```

Global cables use `RuntimeTarget::GlobalCable`.

### Connection flow (compiled node)

1. `GlobalRoutingManager::connectToRuntimeTargets(OpaqueNode& on, bool shouldAdd)`
   iterates all cables and calls `on.connectToRuntimeTarget(add, cable.createConnection())`
2. The OpaqueNode forwards to the wrapped node's `connectToRuntimeTarget(add, c)`
3. The `indexable_target` checks if `c.getType() == GlobalCable` and
   `c.getHash() == index.getIndex()`
4. On match: disconnects from any previous source, connects to the new one
5. On connect, `Cable::connectStatic<true>()` adds the typed_target to the
   cable's RuntimeTarget aggregator and sends pending lastData

### Hash-based addressing

Cables are identified by string ID in the manager, but compiled nodes use
integer hash codes (`String::hashCode()`). The `fix_hash<HashIndex>` indexer
provides a compile-time constant hash. The `dynamic` indexer allows runtime
hash changes.

---

## 7. Thread Safety

### Cable value routing

- `Cable::sendValue()` iterates the target list **without acquiring a lock**.
  The targets list is only modified under write lock, but iteration during
  sends is lock-free. This is safe because:
  - `CableTargetBase::List` is `Array<WeakReference<CableTargetBase>>`
  - Individual target calls are atomic from the cable's perspective
  - Stale weak references are cleaned up during `cleanup()`

- `Cable::addTarget()` / `removeTarget()` acquire `SimpleReadWriteLock::ScopedWriteLock`
  on `SlotBase::lock`.

### Signal audio routing

- Both `push()` and `pop()` use `ScopedTryReadLock` on `SlotBase::lock`
  (non-blocking). If the lock cannot be acquired, the operation is silently
  skipped -- no audio is copied.

- `setSource()` uses `ScopedWriteLock` on `SlotBase::lock` for buffer
  reallocation.

### Node connection changes

- `GlobalRoutingNodeBase::updateConnection()` acquires `ScopedWriteLock` on
  `connectionLock` (per-node lock, separate from slot lock).

- `GlobalRoutingNodeBase::prepare()` acquires `ScopedReadLock` on
  `connectionLock`.

- `GlobalSendNode::process()` and `GlobalReceiveNode::process()` use
  `ScopedTryReadLock` on `connectionLock` -- audio thread never blocks.

### Lock hierarchy

Two independent lock domains:
1. **SlotBase::lock** -- protects slot internals (target list, buffer)
2. **Node::connectionLock** -- protects the node's currentSlot/currentCable pointer

Both use `SimpleReadWriteLock` with try-read semantics on the audio thread.

---

## 8. OSC Integration

Global cables can be controlled via OSC. The manager holds optional
`HiseOSCReceiver` and `HiseOSCSender` objects.

### Receiving OSC

- OSC messages matching the configured domain are routed to cables whose ID
  starts with `/`
- Values are converted to double (float32 or int32 OSC types)
- Input ranges from `OSCConnectionData::inputRanges` are applied to normalize
  to 0..1
- Values outside -0.1..1.1 after normalization trigger an error
- Only cables with IDs starting with `/` receive OSC messages

### Sending OSC

- When an OSC sender is active, cables with `/`-prefixed IDs get an
  `OSCCableTarget` added automatically
- Output ranges from `OSCConnectionData::inputRanges` are applied to
  denormalize from 0..1
- The `sendOSCMessageToOutput()` method allows sending arbitrary OSC messages

---

## 9. Key Differences: Global vs Local Routing

| Aspect | Local (send/receive) | Global (global_send/global_receive) |
|--------|---------------------|-------------------------------------|
| Scope | Within one network | Across all networks in the instance |
| Naming | Variable-based | String ID-based slots |
| Management | ValueTree connections | GlobalRoutingManager singleton |
| Audio | Direct buffer reference | Copy into shared buffer (push/pop) |
| Latency | Zero | Zero (same audio callback) |
| Compilation | Compileable to C++ | **Not compileable** (UncompileableNode) |
| Channel matching | Automatic | Must match exactly (error otherwise) |
| Block size | Same context | Receiver blockSize <= sender blockSize |

| Aspect | Local cable | Global cable |
|--------|------------|--------------|
| Scope | Within one network | Across all networks + C++ nodes |
| Value range | Arbitrary | Clamped to 0..1 |
| Data support | No | Yes (binary data via sendData) |
| OSC support | No | Yes (for `/`-prefixed IDs) |
| Compilation | Compileable | Compileable (via runtime_target) |
| Connection | Compile-time | Runtime (hash-based) |

---

## 10. Error Conditions

| Error | Trigger | Context |
|-------|---------|---------|
| `Error::NoGlobalCable` | Compiled global_cable not connected at prepare() | Only when `IndexType::mustBeConnected()` is true |
| `Error::SampleRateMismatch` | Receiver sample rate != sender sample rate | Signal slot addTarget |
| `Error::ChannelMismatch` | Receiver channels != sender channels | Signal slot addTarget |
| `Error::BlockSizeMismatch` | Receiver blockSize > sender blockSize | Signal slot addTarget |
| "Slot already has a send node" | Second send node tries to connect | Signal slot setSource |
| "Unconnected" | No slot selected or no send node | Signal slot connection status |

---

## 11. CustomNodeProperties for Global Routing Nodes

| Node | Properties |
|------|-----------|
| GlobalCableNode (IDE) | `IsControlNode`, `IsFixRuntimeTarget` |
| GlobalSendNode | `UncompileableNode` |
| GlobalReceiveNode | `UncompileableNode` |
| global_cable (compiled) | `IsControlNode` (via no_processing base), `IsFixRuntimeTarget` (via indexable_target) |
