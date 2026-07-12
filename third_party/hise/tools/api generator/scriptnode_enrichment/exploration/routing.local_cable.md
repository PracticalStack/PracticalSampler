# routing.local_cable - C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/dynamic_elements/DynamicRoutingNodes.h:698` (node), `DynamicRoutingNodes.h:448` (base class), `DynamicRoutingNodes.cpp:351` (setValue)
**Base class:** `local_cable_base` -> `mothernode` + `control::pimpl::parameter_node_base<parameter::dynamic_base_holder>` + `control::pimpl::no_processing`
**Classification:** control_source

## Signal Path

No audio processing. Routes a control value to other local_cable nodes sharing the same LocalId within a single DspNetwork. When `setValue(v)` is called:

1. Checks recursion guard (`if(!recursion)`)
2. Sets `ScopedValueSetter<bool>(recursion, true)`
3. Calls `sendValue(v)` -- iterates all connected nodes and sets their first parameter to `v` via `setValueAsync()`
4. Calls `getManager()->setVariableValue(variableIndex, v)` -- stores value in the Manager's shared variable slot
5. Calls `this->getParameter().call(v)` -- sends to the modulation output

## Gap Answers

### empty-description: The base data description is empty. What is an accurate description of this node?

Based on the C++ source: "Routes a normalised control value between nodes sharing the same LocalId within a single DspNetwork."

### local-cable-value-flow: How does the value flow between local_cable nodes sharing the same LocalId? When one node's Value parameter changes, do all others update? Is there a recursion guard like global_cable?

Yes, bidirectional with recursion guard. When any local_cable's Value parameter changes, `setValue(v)` broadcasts to all other local_cable nodes with the same LocalId via `sendValue()`. The recursion guard (`bool recursion` + `ScopedValueSetter`) prevents infinite loops -- if a cable receives a value from another cable, the `recursion` flag blocks it from re-broadcasting.

The `sendValue()` method acquires a `SimpleReadWriteLock::ScopedReadLock` on the node's `lock` and iterates the `connections` array (which contains `WeakReference<NodeBase>` pointers to other local_cable nodes), calling `setValueAsync(v)` on each one's first parameter.

### scope-limitation: Is the local cable strictly scoped to one DspNetwork, or can it cross into nested networks? What is the exact scope boundary?

Strictly scoped to one DspNetwork. The Manager is obtained via `node->getRootNetwork()->getLocalCableManager()` -- each root DspNetwork has its own Manager instance. The Manager holds up to 64 variable slots (`NumVariableSlots = 64`) keyed by string IDs. Nested networks would have their own root network and thus their own separate Manager.

### compilation-support: Local cables are compileable to C++ (unlike global_send/receive). How are they represented in compiled code -- as direct parameter connections or as a shared variable?

The IDE local_cable class does NOT inherit from `UncompileableNode` -- so it is compileable. However, looking at the implementation, local_cable_base relies on the DspNetwork runtime infrastructure (Manager singleton, ValueTree connections, `setValueAsync`). In compiled C++ code, local cables would likely be converted to direct parameter connections by the C++ code generator, since the variable-sharing semantics can be represented as compile-time parameter wiring. The node is not present in `RoutingNodes.h` (no compiled template equivalent exists), confirming that the code generator handles the conversion.

## Parameters

- **Value** (0..1, default 0): The control value routed through the cable. Setting this broadcasts to all other local_cable nodes with the same LocalId and sends to the modulation output.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

- The Manager stores values in a `span<double, 64>` array indexed by variable slot.
- `refreshConnection()` updates the node's colour based on the LocalId (using `GlobalRoutingManager::Helpers::getColourFromId`).
- The `connections` array is maintained by the Manager when cables register/deregister.
- `isNormalisedModulation()` returns true (inherited from `no_processing` base).
