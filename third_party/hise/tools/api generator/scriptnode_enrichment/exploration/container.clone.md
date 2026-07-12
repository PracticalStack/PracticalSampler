# container.clone -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:220` (interpreted), `hi_dsp_library/node_api/nodes/duplicate.h` (compiled infrastructure)
**Base class:** `SerialNode` (interpreted), `wrap::clone_base<DynamicCloneData, CloneProcessType::Dynamic>` (wrapper)
**Classification:** container

## Signal Path

An array of identical child node chains processed in parallel or serially.
The `SplitSignal` parameter selects between three processing modes:

**Serial (SplitSignal=0):** Clones process sequentially on the same buffer.
Clone 0 gets the original input, clone 1 gets clone 0's output, etc.
```
Input -> Clone[0].process -> Clone[1].process -> ... -> Clone[N].process -> Output
```

**Parallel (SplitSignal=1):** Each clone receives a zeroed buffer, processes it,
and the output is added to the original signal (which passes through unmodified).
```
Input --+--[passthrough]--> Output (original preserved)
        |
        +--Clone[0](silence) --add--> Output
        +--Clone[1](silence) --add--> Output
```

**Copy (SplitSignal=2, default):** Each clone receives a copy of the original input.
All clone outputs are summed. The original signal is NOT preserved.
```
Input --copy--> each Clone[i]
Output = sum of all Clone[i] outputs (original discarded)
```

## Gap Answers

### split-signal-modes: Confirm three CloneProcessType modes

**Confirmed.** The `CloneProcessType` enum in duplicate.h defines Serial=0,
Parallel=1, Copy=2. The interpreted CloneNode always uses `CloneProcessType::Dynamic`
(NodeContainerTypes.h:337), which allows runtime switching via the SplitSignal parameter.

`setSplitSignal()` (NodeContainerTypes.cpp:1168-1172) calls `obj.setCloneProcessType()`
and also updates the `isVertical` UI property (Serial=vertical, Parallel/Copy=horizontal).

The `createInternalParameterList()` (lines 1100-1119) confirms:
- SplitSignal range {0, 2, 1}, default 2.0 (Copy)
- Labels: "Serial", "Parallel", "Copy"

### clone-resize-threading: Thread safety model

**Confirmed.** The `clone_manager` base class uses a `SimpleReadWriteLock`.
Audio processing methods (`process`, `processFrame`, `reset`, `handleHiseEvent`)
acquire `ScopedTryReadLock` -- if resize is in progress, processing is silently
skipped for that buffer (duplicate.h, documented in infrastructure/duplicate.md).

`prepare()` acquires a blocking read lock. `resetCopyBuffer()` (called when
process type changes or during prepare) acquires a blocking write lock.

The audible effect of a clone resize is a brief silence/gap for one buffer
(typically 512 samples / ~11ms at 44.1kHz).

### inactive-clone-handling: Inactive clones in interpreted CloneNode

**Confirmed.** `setNumClones()` (NodeContainerTypes.cpp:1157-1166) bypasses
individual clone nodes: `nodes[i]->setBypassed(i >= newSize)`. All clones remain
in the node tree (not destroyed). Bypassed clones are still prepared via
`prepareNodes(ps)` in `prepare()` (line 1141), so they consume memory for their
internal buffers but no CPU during process (skipped by ActiveIterator).

### frame-processing-support: Frame processing limitation

**Confirmed.** `CloneNode::processFrame()` contains `jassertfalse`
(NodeContainerTypes.cpp:1121-1125). Clone containers cannot be used inside
frame-based containers (frameN_block, framex_block). The assertion will fire
in debug builds; in release, the function does nothing.

### numclones-range-dynamic: NumClones parameter range

**Confirmed.** The initial NumClones range is {1, 16, 1} with default 1
(NodeContainerTypes.cpp:1106). However, the `numVoicesListener` callback
(lines 1059-1064) dynamically updates the MaxValue when clones are added/removed:
`numTree.setProperty(PropertyIds::MaxValue, numMax, ...)` where
`numMax = jmax(1, getNodeTree().getNumChildren())`.

The base data showing max=1 reflects the initial state with a single clone_child.
It grows dynamically as clones are duplicated via the UI toolbar.

## Parameters

- **NumClones** (P=0): Number of active clones. Range 1 to totalClones (dynamic),
  step 1, default 1. Inactive clones are bypassed, not destroyed.
- **SplitSignal** (P=1): Processing mode selector. 0=Serial, 1=Parallel, 2=Copy.
  Default 2 (Copy). Also controls UI layout direction.

## Conditional Behaviour

The SplitSignal parameter selects between three fundamentally different audio
routing modes (see Signal Path above). Each mode has different buffer requirements:
- Serial: no extra buffers (in-place)
- Parallel: work buffer (zeroed per clone)
- Copy: work buffer + original buffer

When bypassed, only the first clone processes audio:
`if (isBypassed() && !nodes.isEmpty()) nodes.getFirst()->process(data)`
(NodeContainerTypes.cpp:1132-1133).

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: [
  { "parameter": "NumClones", "impact": "linear", "note": "each active clone runs a full process pass" },
  { "parameter": "SplitSignal", "impact": "mode-dependent", "note": "Parallel/Copy modes add buffer copy+add overhead; Serial is cheapest" }
]

## Notes

- `HasFixedParameters = true` -- parameters cannot be added/removed.
- Clone requires `control.clone_cable`, `control.clone_forward`, or
  `control.clone_pack` for per-clone parameter differentiation. Without these,
  all clones receive identical parameter values.
- The constructor creates a default child: `container.chain` with pink color
  (0xFF949494) named `{id}_child` (NodeContainerTypes.cpp:1088-1097).
- CloneNode validates that all clones match the first clone's structure via
  `checkValidClones()` (lines 1247-1272). Mismatches produce `Error::CloneMismatch`.
- The root element of each clone must be a container (line 1253).
- Property syncing: `valueSyncer` and `uiSyncer` listeners propagate property
  changes from one clone to all siblings via `syncCloneProperty()`.
- Connection syncing: `connectionListener` mirrors parameter connections and
  modulation targets across clones via `updateConnections()`.
- The `DisplayedClones` property controls which clones are visible in the UI,
  supporting range syntax like "1-3,5" (lines 1274-1306).
