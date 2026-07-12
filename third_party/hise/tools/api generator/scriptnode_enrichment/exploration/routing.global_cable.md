# routing.global_cable - C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingNodes.h:114` (IDE), `hi_dsp_library/dsp_nodes/RoutingNodes.h:1154` (compiled)
**Base class:** IDE: `ModulationSourceNode` + `GlobalRoutingManager::CableTargetBase`; Compiled: `mothernode` + `control::pimpl::no_processing` + `control::pimpl::parameter_node_base<ParameterClass>` + `runtime_target::indexable_target`
**Classification:** control_source

## Signal Path

No audio processing. The node routes a normalised 0..1 control value through the GlobalRoutingManager Cable system. When the Value parameter is set locally, it broadcasts to all other global_cable nodes on the same Connection. When a value arrives from another source on the cable, it is forwarded to the node's modulation output (parameter target).

IDE flow: `setValue()` -> stores `lastValue` -> `currentCable->sendValue(this, v)` -> Cable broadcasts to all other targets (excluding sender) -> each target's `sendValue(v)` calls `getParameterHolder()->call(v)`.

Compiled flow: `setValue(newValue)` -> sets `recursion = true` -> `sendValueToSource(newValue)` -> Cable dispatches to all other targets -> `onValue(c)` on receiving end checks recursion guard, then calls `this->getParameter().call(c)`.

## Gap Answers

### bidirectional-value-flow: The global_cable acts as both sender and receiver on the same cable. When one global_cable node sets a value, how does it avoid feedback loops? Is there a recursion guard?

Yes, there is a recursion guard in both IDE and compiled implementations.

**IDE (GlobalCableNode):** `Cable::sendValue(source, v)` iterates all targets except the source (`t != source`). So the sending node never receives its own value back. See `GlobalRoutingManager.cpp` Cable::sendValue -- it skips the source pointer.

**Compiled (routing::global_cable):** Uses a `bool recursion` member. `setValue()` sets `ScopedValueSetter<bool> rec(recursion, true)` before calling `sendValueToSource()`. When `onValue(c)` is called from the cable, it checks `if(recursion) return;` -- preventing the node from forwarding a value it just sent.

### empty-description: The base data description is empty. What is a concise, accurate description of this node's purpose?

Based on the C++ source: "Routes a normalised 0..1 control value across all DspNetworks via the GlobalRoutingManager cable system."

### binary-data-transmission: The compiled global_cable supports sendData() for binary data transmission. Is this feature accessible from the interpreted (IDE) global_cable node, or only from C++ compiled code?

The compiled `routing::global_cable` template has `sendData(data, numBytes)` and `onData(data, numBytes)` methods, plus a `setDataCallback()` registration. The IDE `GlobalCableNode` class does NOT expose sendData/onData -- it only has `sendValue(double v)`. Binary data transmission is only available from compiled (C++) code, typically via the `global_cable_cpp_manager` helper class which provides `sendDataToGlobalCable<CableIndex>(var)`.

### osc-integration: Global cables with IDs starting with '/' can be controlled via OSC. Is this documented behaviour or implementation detail? What input/output range mapping is applied?

This is implemented in `GlobalRoutingManager`. When an OSC receiver is active, cables with `/`-prefixed IDs receive OSC messages. Input range mapping is applied via `OSCConnectionData::inputRanges` to normalise incoming OSC values to 0..1. Values outside approximately -0.1..1.1 after normalisation trigger an error. For output: when an OSC sender is active, cables with `/`-prefixed IDs get an `OSCCableTarget` that denormalises from 0..1 using the configured output ranges. This is a real, supported feature -- not an implementation detail.

## Parameters

- **Value** (0..1, default 1.0): The control value to send/receive. Setting this parameter calls `setValue()` which broadcasts to the cable. The value is always clamped to 0..1 by the Cable infrastructure (`jlimit(0.0, 1.0, v)`).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

- The IDE node inherits from `ModulationSourceNode`, giving it modulation output capability via `getParameterHolder()`.
- The compiled node uses `runtime_target::indexable_target` with hash-based addressing (`fix_hash<hash>` or `dynamic` indexer) to connect to the GlobalRoutingManager across DLL boundaries.
- `process()` and `processFrame()` are empty in both implementations -- this is purely a control node.
- The `SN_GLOBAL_CABLE(hash)` macro creates a `routing::global_cable<runtime_target::indexers::fix_hash<hash>, parameter::empty>` for use in compiled nodes.
- `isUsingNormalisedRange()` returns true on the IDE node.
- Compile-time validation: if `IndexType::mustBeConnected()` is true and the cable is not connected, `prepare()` throws `Error::NoGlobalCable`.
