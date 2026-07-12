# container.chain -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/Container_Chain.h:170` (compiled), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:40` (interpreted)
**Base class:** `container_base<ParameterClass, Processors...>` (compiled), `SerialNode` (interpreted)
**Classification:** container

## Signal Path

Serial in-place processing. Input buffer passes through each child in list order.
Each child modifies the buffer in-place; the next child sees the modified result.

```
Input -> Child[0].process(data) -> Child[1].process(data) -> ... -> Child[N].process(data) -> Output
```

No buffer copying or allocation required. The same ProcessData reference is passed
to every child.

## Gap Answers

### child-dispatch-order: Confirm serial list-order iteration

**Confirmed.** The compiled version uses `call_tuple_iterator1(process, p)` which
expands to a fold expression over `std::index_sequence` (Container_Chain.h:200-201).
The `swallow` array trick in the tuple iterator macro (parameter.h) guarantees
strict left-to-right evaluation. Index 0 processes first, last index processes last.

The interpreted `ChainNode` delegates to `bypass::simple<DynamicSerialProcessor>`.
`DynamicSerialProcessor` iterates `parent->getNodeList()` with a standard for-each
loop (NodeContainer.h), which traverses the `NodeBase::List` (ReferenceCountedArray)
in index order.

No reordering or parallel execution occurs in either path.

### bypass-wrapper-type: Confirm bypass::simple hard bypass behavior

**Confirmed.** `ChainNode` wraps `DynamicSerialProcessor` in `bypass::simple`
(NodeContainerTypes.h:42). The `ChainNode::process()` method also checks
`isBypassed()` at the top and returns immediately (NodeContainerTypes.cpp:56-57),
providing a second bypass check before the wrapper.

When bypassed:
- `bypass::simple::process()` is a no-op (audio passes through unmodified)
- `bypass::simple::setBypassed(true)` calls `reset()` on the inner processor
- No crossfade -- transition is instantaneous (may click with stateful effects)

This contrasts with `container.soft_bypass` which uses `bypass::smoothed` for
click-free transitions.

### event-forwarding: HiseEvent handling in chain

**Confirmed.** The compiled chain passes the same `HiseEvent&` reference to all
children via `call_tuple_iterator1(handleHiseEvent, e)` (Container_Chain.h:213).
Children receive events in list order (same guarantee as process).

If the first child modifies the event (e.g., changes channel, velocity, timestamp),
subsequent children see the modified version. This is by design for serial containers.

The interpreted `ChainNode::handleHiseEvent()` delegates to
`wrapper.handleHiseEvent(e)` (NodeContainerTypes.cpp:98), which forwards through
the bypass::simple wrapper to DynamicSerialProcessor, which iterates children
in order passing the same reference.

## Parameters

None. Chain has no parameters of its own.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

Chain adds zero processing overhead -- it is a pure dispatch mechanism.
All CPU cost comes from the child nodes.

## Notes

- The `IsVertical` property is purely cosmetic (UI layout direction).
- The compiled version's channel count is determined by the first child's
  `NumChannels` template parameter. The interpreted version uses the network's
  dynamic channel count.
- ChainNode has a `bypassListener` member (valuetree::PropertyListener) for
  tracking bypass state changes from the ValueTree.
