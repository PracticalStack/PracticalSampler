# container.branch -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/Container_Chain.h:241` (compiled), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:871` (interpreted)
**Base class:** `container_base<parameter::branch_index<N>, Processors...>` (compiled), `ParallelNode` (interpreted)
**Classification:** container

## Signal Path

Index-selected single-child processing. Only the child at `currentIndex` receives
process(), processFrame(), and handleHiseEvent() calls. All other children are idle.

```
Input --> switch(currentIndex) --> Child[index].process(data) --> Output
```

Audio passes through the selected child in-place. Unselected children do not
process audio but ARE fully prepared and can be switched to at any time.

## Gap Answers

### index-clamping-behavior: Index clamping and float handling

**Compiled version:** Uses `jlimit<uint8>(0, N-1, roundToInt(v))` in
`parameter::branch_index::call()` (Container_Chain.h:51). Out-of-range values
are clamped to [0, N-1]. Float values are rounded to nearest integer.

The CASE macro uses `jmin(NumElements-1, idx)` (line 222), which ensures that
switch cases beyond the actual number of children map to the last child.
This is a compile-time safety measure for the switch/case expansion.

**Interpreted version:** `BranchNode::setIndex()` uses `roundToInt(s)` and
stores as `int currentIndex` (NodeContainerTypes.cpp:619-622). The processing
methods access `nodes[currentIndex]` which returns nullptr for out-of-range
indices (ReferenceCountedArray behavior). The null check `if(auto n = nodes[currentIndex])`
at lines 639, 645, 654 prevents crashes but means out-of-range indices produce
silence (no child processes).

`updateIndexLimit()` (lines 605-617) dynamically adjusts the Index parameter's
max value when children are added/removed: sets MaxValue to `numChildren - 1`.
If the current value exceeds the new max, it is clamped down.

### inactive-children-state: Prepare/reset for inactive children

**Compiled version confirmed:** `prepare()` uses `call_tuple_iterator1(prepare, ps)`
which prepares ALL children (Container_Chain.h:268). Similarly, reset() in the
compiled version is NOT shown but inherited from container_base which resets all.

**Interpreted version confirmed:** `BranchNode::prepare()` calls
`NodeContainer::prepareNodes(ps)` which prepares all children (NodeContainerTypes.cpp:624-629).
`BranchNode::reset()` iterates all nodes: `for(auto n: nodes) n->reset()` (lines 631-635).

Only process/processFrame/handleHiseEvent are index-selective.

**Switching mid-buffer:** Index changes take effect immediately (no smoothing).
Switching mid-buffer will cause the new child to start processing from wherever
its internal state was left (post-reset from prepare). This can cause clicks
with stateful effects. For click-free switching, use `template.softbypass_switchN`
which combines soft_bypass containers with control.xfader.

### max-children-limit: Maximum children

**Compiled:** `static_assert(NumElements <= 16)` in prepare() (Container_Chain.h:266).
Maximum 16 children, enforced at compile time.

**Interpreted:** No hard limit. The `nodes` ReferenceCountedArray can hold any number.
The Index parameter default max is 10 (line 664: `p.setRange({0.0, 10.0, 1.0})`),
but `updateIndexLimit()` dynamically adjusts this when children are added/removed.

### bypass-behavior: Branch bypass

**Confirmed.** `BranchNode::process()` checks `isBypassed()` at entry and returns
immediately (NodeContainerTypes.cpp:651-652). When bypassed, audio passes through
unmodified -- none of the children process.

## Parameters

- **Index** (P=0): Selects which child processes audio. Range 0 to N-1 (dynamic max),
  step 1, default 0. Float values are rounded. Out-of-range values: clamped in
  compiled version, produce silence in interpreted version.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

Branch has minimal overhead -- one switch/case dispatch (compiled) or one array
lookup (interpreted) per callback. Only the selected child's CPU cost applies.
All children's prepare/reset overhead is always incurred.

## Notes

- `HasFixedParameters = true` in cppProperties -- parameters cannot be added/removed.
- The compiled `branch` uses `SN_EMPTY_SET_EXTERNAL_DATA` (Container_Chain.h:289),
  meaning external data is not forwarded through the branch container itself.
- The `indexRangeUpdater` child listener in BranchNode monitors additions/removals
  to the Nodes child tree (NodeContainerTypes.h:908).
