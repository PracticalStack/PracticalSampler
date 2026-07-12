# control.resetter - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:858`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

Value parameter changes -> setParameter<0>() -> sends 0.0 then 1.0 in rapid succession to output parameter.

## Gap Answers

### impulse-mechanism: What is the exact impulse sequence and trigger condition?

The `setParameter<0>(double v)` method ignores the input value entirely. On every call:
1. Increments `flashCounter` (used for UI display)
2. Calls `this->getParameter().call(0.0)` -- sends 0
3. Calls `this->getParameter().call(1.0)` -- sends 1

The sequence is always 0 then 1, sent synchronously within the same callback. Any parameter change triggers this -- there is no edge detection or threshold check.

### reset-target-pattern: What are "gate-like parameters"?

The 0-then-1 sequence is designed to re-trigger any parameter that responds to transitions. For example, an envelope gate parameter that starts on a 0->1 transition will be reset by receiving 0 followed by 1. This forces a re-trigger regardless of the gate's current state. It works for envelope gates, toggle switches, or any parameter that responds to rising edges.

## Parameters

- **Value**: Trigger input. The actual value is completely ignored; any change fires the 0->1 impulse pair.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The `flashCounter` member is incremented on each trigger but never reset in the node code. It is likely used by the UI to animate a flash indicator. The node uses `SN_FORWARD_PARAMETER_TO_MEMBER` which routes through `setParameter<P>()` rather than using `SN_ADD_SET_VALUE`.
