# control.locked_mod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1192`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

Value parameter input -> setValue() -> direct passthrough to output parameter (normalised).

locked_mod is a simple passthrough control node. The `setValue()` method forwards the input value unchanged to the output parameter via `this->getParameter().call(input)`.

## Gap Answers

### locked-container-interaction: How does locked_mod discover and connect to its parent locked container?

The C++ class itself has no container discovery logic. The connection to the parent locked node container is handled at the runtime level by the scriptnode ValueTree/connection system, not in the node's C++ code. The node is a standard `parameter_node_base` with a single output. The "locked container parent" relationship is established by the hosting infrastructure when the node is placed inside a locked container.

### passthrough-behaviour: Does locked_mod simply pass through the Value unchanged?

Yes. The `setValue(double input)` method calls `this->getParameter().call(input)` with no transformation. Since `isNormalisedModulation()` defaults to true (from `no_processing`), the connection system applies target parameter ranges to convert the 0..1 input to the target's actual range. Output is always normalised 0..1.

## Parameters

- **Value**: Input value (0..1), forwarded unchanged to output.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
