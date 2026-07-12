# Scriptnode Clone/Duplicate Infrastructure Reference

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/node_api/nodes/duplicate.h`
- `hi_dsp_library/dsp_nodes/CableNodes.h` (clone_cable, clone_forward, clone_pack)
- `hi_dsp_library/dsp_basics/logic_classes.h` (duplilogic namespace)
- `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h` (CloneNode)
- `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.cpp` (CloneNode impl)
- `hi_scripting/scripting/scriptnode/dynamic_elements/DynamicRoutingNodes.h` (clone_holder, dynamic duplilogic)
- `hi_scripting/scripting/scriptnode/dynamic_elements/DynamicParameterList.cpp` (clone_holder impl)

Also loaded: `scriptnode_enrichment/resources/infrastructure/core.md`

---

## 1. Overview: The Clone System

The clone system allows a single child node chain to be duplicated N times and
processed in parallel (or serially). This enables unison voices, stacked
oscillators, parallel filter banks, and similar multi-instance patterns without
manually duplicating nodes.

Key components:
- **container.clone** -- the container that holds and processes N copies of its
  child chain
- **control.clone_cable** -- distributes per-clone parameter values using a
  mathematical distribution (spread, scale, harmonics, etc.)
- **control.clone_forward** -- forwards a single unscaled value identically to
  all clones
- **control.clone_pack** -- distributes values from a SliderPack to individual
  clones

---

## 2. CloneProcessType Enum

Defined in `duplicate.h`. Controls how audio flows through the clones.

```cpp
enum class CloneProcessType
{
    Serial,   // Processes clones serially with the original signal
    Parallel, // Processes clones with empty input, adds output to original
    Copy,     // Copies input to each clone, sums all outputs
    Dynamic   // Allows runtime switching between the above modes
};
```

### Audio routing per mode

| Mode | Input to each clone | Output mixing | Work buffer needed | Original buffer needed |
|---|---|---|---|---|
| Serial | Previous clone's output (first clone gets original) | Chain: output of last clone | No | No |
| Parallel | Silence (zeroed buffer) | Sum: all clone outputs added to original | Yes | No |
| Copy | Copy of original input | Sum: all clone outputs (original is discarded) | Yes | Yes |
| Dynamic | Depends on runtime processType value | Same as selected mode | Yes (if non-Serial) | Yes (if Copy) |

### Serial mode detail

Each clone's `process()` is called sequentially on the same ProcessData buffer.
Clone 0 processes the input, clone 1 processes clone 0's output, and so on.
This is useful for serial filter chains or cascaded effects.

### Parallel mode detail

A zeroed work buffer is created. Each clone processes the work buffer
independently, and the result is added (FloatVectorOperations::add) to the
original signal. The original signal passes through unchanged; clone outputs
are layered on top. This is the typical unison mode.

### Copy mode detail

The original input is saved to a separate buffer. Then for each clone, the
work buffer is filled with the original input, the clone processes it, and
the result is added to the output. The output buffer is cleared before
accumulation, so the result is purely the sum of all clone outputs -- the
original signal does NOT pass through unmodified (unlike Parallel).

### Dynamic mode

Only used by the interpreted CloneNode (not compiled nodes). The CloneNode
always instantiates `clone_base<DynamicCloneData, CloneProcessType::Dynamic>`.
The SplitSignal parameter maps to the CloneProcessType enum via
`setCloneProcessType(double)`.

---

## 3. clone_manager: Clone Count Management

`wrap::clone_manager` is the central authority for clone count. It is the base
class of `clone_base` (and thus container.clone).

### Key members

```
int numClones          -- current active clone count
SimpleReadWriteLock    -- protects clone resize operations
Array<WeakRef<Listener>> listeners -- notified on count changes
```

### setNumClones(int newSize)

- Clamps to `[1, getTotalNumClones()]`
- If changed, stores new value and calls `sendMessageToListeners()`
- `getTotalNumClones()` returns the maximum number of clones (compile-time for
  exported nodes, `nodes.size()` for interpreted CloneNode)

### Listener interface

```cpp
struct Listener {
    virtual void numClonesChanged(int newNumClones) = 0;
};
```

All three clone control nodes (clone_cable, clone_forward, clone_pack)
implement this listener. When a control node connects to a clone container's
parameter system, it registers itself as a listener via
`setParentNumClonesListener()`. This means clone control nodes automatically
track the container's clone count.

### Thread safety

All audio processing methods (`process`, `processFrame`, `reset`,
`handleHiseEvent`) acquire a `ScopedTryReadLock` on the clone resize lock.
If the lock cannot be acquired (because a resize is in progress), processing
is silently skipped for that buffer. The `prepare()` method acquires a
blocking read lock.

---

## 4. clone_base: The Container Implementation

`clone_base<DataType, ProcessType>` inherits from `clone_manager` and
implements the full node interface (prepare, process, processFrame, reset,
handleHiseEvent).

### DataType template parameter

For compiled/exported nodes: `clone_data<T, AllowResizing, NumClones>` --
stores clones as a C array `T data[NumClones]`.

For interpreted nodes: `DynamicCloneData` -- stores clones as an
`Array<WeakReference<NodeBase>>` inside CloneNode. The DynamicCloneData
uses `NodeWrapper` (a WeakReference<NodeBase> subclass) that forwards all
calls to the referenced NodeBase.

### Iterator system

Two iterator types control which clones are processed:

- **ActiveIterator** (`Iterator<false>`) -- iterates `[0, numClones)`. Used in
  process/reset/handleHiseEvent -- only active clones are processed.
- **AllIterator** (`Iterator<true>`) -- iterates `[0, NumClones)` (all allocated).
  Used in prepare/initialise -- all clones must be prepared even if inactive.

### Parameters

clone_base has two parameters accessed via `setParameterStatic<P>`:
- P=0: NumClones -- calls `setNumClones(v)`
- P=1: SplitSignal -- calls `setCloneProcessType(v)`

### CloneNode (interpreted) specifics

The interpreted CloneNode in `NodeContainerTypes.cpp`:
- `setNumClones()` bypasses individual clone nodes (i >= newSize) rather than
  destroying them. All clones always exist; inactive ones are bypassed.
- `setSplitSignal()` maps to `CloneProcessType` and also stores an `isVertical`
  property for UI layout.
- When bypassed, only the first clone processes audio (bypass passthrough).
- `processFrame()` is not implemented (asserts false) -- frame processing goes
  through `process()`.

---

## 5. cloned_node_reference: Per-Clone Parameter Addressing

`cloned_node_reference<T>` is the compile-time mechanism for addressing the
same parameter across all clones. It uses pointer arithmetic to navigate
between clones.

### How it works

```
firstObj   -- pointer to the target node in clone 0
objectDelta -- sizeof(one complete clone chain) in bytes
cloneManager -- pointer to the clone_manager (for clone count)
```

To reach clone N's instance: `(uint8*)firstObj + objectDelta * N`

This works because compiled clone containers store clones in a contiguous
C array. Each clone is the same type with the same size, so pointer arithmetic
with a fixed stride reaches the equivalent node in each clone.

### get<P>() -- navigation

`get<P>()` navigates into a child node (like a container's get<P>()). It
returns a new `cloned_node_reference` pointing to the P-th child in the first
clone, with the same objectDelta. This allows chained navigation like
`cloneRef.get<0>().get<2>()` to reach a specific nested node across all clones.

### connect<P>(Target&)

Connects modulation output from each clone's node to the corresponding clone
in the target. Both source and target must share the same cloneManager
(asserted). Iterates all clones (getTotalNumClones) and wires up individual
connections.

### setParameter<P>(double v)

Sends the same value to all active clones (uses getNumClones, not total).

### setExternalData

Sends the same external data to ALL clones (uses getTotalNumClones).

---

## 6. parameter::cloned -- Clone Parameter Wrapper

`parameter::cloned<ParameterClass>` wraps a regular parameter to make it
clone-aware. It is the compile-time equivalent of `clone_holder`.

### callEachClone(int index, double v, bool ignoreCurrentNumClones)

Calls the wrapped parameter on a specific clone by index:
1. Calculates `thisPtr = (uint8*)firstObj + index * objectDelta`
2. Sets the parameter's object pointer to thisPtr
3. Calls `p.call(v)`

This is the core mechanism by which clone_cable/clone_forward/clone_pack
send different values to different clones.

### connect<P>(CloneRefType& t)

Connects to a `cloned_node_reference`. Copies the target's `firstObj`,
`objectDelta`, and `cloneManager`. If a parentListener was already registered,
re-registers it with the new clone manager.

### isNormalised flag

Controls whether the parameter value should be scaled through the target's
range. Default is `true`. clone_forward sets this to `false` because it
explicitly passes unscaled values.

---

## 7. parameter::clonechain -- Multi-Target Clone Parameters

`clonechain<CloneParameters...>` aggregates multiple `cloned` parameters into
a tuple. When a clone_cable connects to multiple targets, the compiled version
uses a clonechain to fan out to all of them.

- `callEachClone(index, v, ignore)` -- forwards to all contained parameters
- `setParentNumClonesListener(l)` -- registers the listener on all parameters
- `getNumClones()` -- queries the first parameter's clone count

---

## 8. control.clone_cable -- Per-Clone Value Distribution

`control::clone_cable<ParameterType, LogicType>` distributes computed values
to each clone based on a mathematical distribution function.

### Parameters

| Parameter | Range | Default | Description |
|---|---|---|---|
| NumClones | 1-16 (integer) | 1 | Number of active clones |
| Value | 0.0-1.0 | 0.0 | Input value fed to the distribution |
| Gamma | 0.0-1.0 | 0.0 | Shape/curve modifier for the distribution |

### LogicType (duplilogic modes)

The LogicType template parameter determines the distribution function. Each
mode implements `getValue(int index, int numUsed, double inputValue, double gamma)`.

| Mode | Distribution | Output range | Gamma effect | MIDI-reactive |
|---|---|---|---|---|
| spread | Linear ramp centered at 0.5 | 0..1 | Sine curve blending | No |
| scale | Linear ramp from 0 | 0..inputValue | Power curve | No |
| triangle | V-shape (peak at edges) | 1-input..1 | Sine^2 curve | No |
| harmonics | Integer multiples: (index+1)*value | 1x..Nx | None | Yes (frequency) |
| nyquist | Harmonics with smoothstep rolloff | Attenuated harmonics | Rolloff steepness | Yes (frequency) |
| fixed | Same value to all clones | inputValue | None | Yes (frequency) |
| ducker | 1/numClones gain compensation | Varies | Power curve on gain | No |
| random | Spread + random offset | 0..1 (clamped) | None | Yes (note-on retrigger) |
| toggle | Binary on/off by threshold | 0.0 or 1.0 | None | No |

### MIDI-reactive modes

Modes that inherit from `midi_logic::frequency<0>` (harmonics, nyquist, fixed)
respond to MIDI note-on events by updating the input value to the note
frequency. The `random` mode re-randomizes on note-on without changing the
value.

### shouldUpdateNumClones()

Most modes return `true` -- they re-send values when clone count changes. The
`toggle` mode returns `false` because its on/off pattern depends on the
absolute index, not the clone count.

### Dynamic mode (interpreted)

The `duplilogic::dynamic` class wraps all modes behind a runtime switch. It
uses a `NodePropertyT<String>` named "Mode" (default "Spread") to select the
active mode. The property is stored in the ValueTree and selectable in the UI.

### CustomNodeProperties registered

- `IsCloneCableNode` -- marks this as a clone cable
- `IsProcessingHiseEvent` -- always registered (even if the current LogicType
  does not process events, because the dynamic version might)

---

## 9. control.clone_forward -- Unscaled Value Forwarding

`control::clone_forward<ParameterType>` sends the same unscaled value to all
active clones. Unlike clone_cable, it does not apply any per-clone
distribution -- every clone receives the identical value.

### Parameters

| Parameter | Range | Default | Description |
|---|---|---|---|
| NumClones | 1-16 (integer) | 1 | Number of active clones |
| Value | 0.0-1.0 | 0.0 | Value forwarded to all clones |

### Key differences from clone_cable

- No Gamma parameter
- No LogicType template -- always sends the same value
- `isNormalisedModulation() = false` -- values are passed through unscaled
- `UseUnnormalisedModulation` property is registered
- The "Value" parameter is registered as an unscaled parameter via
  `CustomNodeProperties::addUnscaledParameter()`
- Does NOT process MIDI events

### sendValue()

Iterates `[0, numClones)` and calls `callEachClone(i, lastValue, false)` for
each. Every clone receives the same `lastValue`.

### Typical use case

Forwarding a parameter (like filter cutoff or gain) to all clones with the
same value. The connected target parameter's own range handles the scaling.

### Note on createParameters()

clone_forward's `createParameters()` uses `DEFINE_PARAMETERDATA(clone_cable, ...)`
rather than `DEFINE_PARAMETERDATA(clone_forward, ...)`. This means the parameter
names come from clone_cable's static ID, not clone_forward's. This appears to
be a copy-paste artifact but is functionally harmless since parameter names are
resolved by index, not by the source class name.

---

## 10. control.clone_pack -- SliderPack-Driven Clone Control

`control::clone_pack<ParameterType>` reads values from a SliderPack and
multiplies them with an input value to produce per-clone parameter values.

### Parameters

| Parameter | Range | Default | Description |
|---|---|---|---|
| NumClones | 1-16 (integer) | 1 | Number of active clones |
| Value | 0.0-1.0 | 1.0 | Global multiplier applied to all slider values |

### External data

Requires one SliderPack data slot. Each slider index maps to a clone index.
The slider value is multiplied by the Value parameter to produce the final
value sent to each clone:

```
valueForClone[i] = sliderData[i] * lastValue
```

### ContentChange listener

clone_pack registers as a `ComplexDataUIUpdaterBase::EventListener` on the
SliderPack. When a single slider is changed (ContentChange event with the
changed index), only that specific clone's parameter is updated rather than
resending all values. This is an optimization for interactive slider editing.

### setNumClones behavior

When clone count increases, only the newly-added clones receive parameter
updates (iterates from oldNumClones to newNumClones). Existing clones retain
their current values. When clone count decreases, no values are sent -- the
now-inactive clones simply stop being addressed.

### Iteration bound

`numToIterate = jmin(sliderData.size(), numClones)` -- if the SliderPack has
fewer entries than clones, extra clones receive no updates. If the SliderPack
has more entries than clones, the extra slider values are ignored.

---

## 11. clone_holder: Runtime Clone Parameter Routing

`parameter::clone_holder` is the interpreted (runtime) equivalent of
`parameter::cloned`. Used when the scriptnode graph is not compiled.

### How connections are established

1. `setParameter()` is called when a clone control connects to a target
2. It verifies the target node is inside a clone container (`isClone()` check).
   If not, an `Error::CloneMismatch` is raised.
3. It finds the parent CloneNode and calls `rebuild()`
4. It registers a listener on the CloneNode's `cloneChangeBroadcaster` to
   rebuild connections when clones are added/removed

### rebuild() -- connection mirroring

The key operation: given a parameter connection on clone 0, find the
equivalent parameter on all other clones.

1. Uses `CloneNode::CloneIterator` to find sibling ValueTrees across clones
2. For each clone's ValueTree, resolves the corresponding `dynamic_base`
   parameter callback
3. Stores these in `cloneTargets` array (protected by connectionLock)
4. After rebuilding, re-sends all cached `lastValues`

### Constraint: must connect to first clone

`CloneIterator` checks that the connected parameter belongs to clone index 0.
If you connect to a non-first clone, it throws a "You need to connect the
first clone" error.

### callEachClone(index, v, ...)

Looks up `cloneTargets[index]`. If `isNormalised` is true, converts the
value from 0..1 through the target parameter's range. Then calls the
parameter callback.

---

## 12. Thread Safety Model

### Clone resize lock

All audio-path methods in clone_base use `ScopedTryReadLock`. This means:
- If a resize is happening, audio processing is silently skipped (no blocking)
- `prepare()` uses a blocking read lock (called from non-audio thread)
- `resetCopyBuffer()` uses a blocking write lock (called when process type
  changes or during prepare)

### clone_holder connection lock

The runtime clone_holder uses a separate `SimpleReadWriteLock` (connectionLock)
to protect the `cloneTargets` array. `callEachClone` takes a read lock;
`rebuild` takes a write lock.

### Value caching

Both clone_cable and clone_forward cache `lastValue` and `lastGamma`. When
clone count changes, values are re-sent using cached values. clone_holder
maintains an `Array<double> lastValues` with per-clone cached values.

---

## 13. Compiled vs Interpreted Differences

| Aspect | Compiled (exported) | Interpreted (IDE) |
|---|---|---|
| Clone storage | C array: `T data[NumClones]` | `Array<WeakReference<NodeBase>>` |
| Max clones | Fixed at compile time | Dynamic (nodes.size()) |
| Parameter routing | `parameter::cloned` + pointer arithmetic | `clone_holder` + dynamic_base lookup |
| Process type | Template parameter (fixed) or Dynamic | Always Dynamic |
| Clone addressing | Byte offset: `firstObj + index * objectDelta` | Array index into node list |
| Inactive clones | Not processed (ActiveIterator skips) | Bypassed (setBypassed(true)) |
| LogicType | Template parameter (fixed mode) | `duplilogic::dynamic` (runtime property) |

---

## 14. Container Parameters

container.clone exposes two parameters:

| Parameter | Index | Range | Description |
|---|---|---|---|
| NumClones | 0 | 1 to totalClones | Active clone count |
| SplitSignal | 1 | 0=Serial, 1=Parallel, 2=Copy | Audio process type selector |

The SplitSignal parameter directly maps to the `CloneProcessType` enum values.
In the interpreted CloneNode, it also controls the `isVertical` UI property
(Serial = vertical layout, Parallel/Copy = horizontal layout).
