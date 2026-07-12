# control.normaliser - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1142`
**Base class:** `mothernode`, `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

Value parameter (0..1) -> pass through unchanged -> modulation output (normalised).

The node is a simple pass-through. `setValue()` at line 1159:

```
void setValue(double input)
{
    if (this->getParameter().isConnected())
        this->getParameter().call(input);
}
```

No transformation is applied. The "normalisation" happens in the connection layer -- since `isNormalisedModulation()` returns true (from `no_processing` base), the connection system applies target parameter ranges to the 0..1 output.

## Gap Answers

### normaliser-passthrough-verification: Is control.normaliser a simple pass-through (Pattern 1)?

Confirmed. It is exactly Pattern 1 (simple pass-through) from the control infrastructure. The `setValue()` method (line 1159) forwards the input value directly to the output without any transformation. The node's purpose is to serve as a normalised cable -- it accepts a 0..1 value and the connection system's range conversion does the actual normalisation work when delivering to targets.

### normaliser-not-polyphonic: Does normaliser use multi_parameter or a simpler pattern?

Normaliser does NOT use `multi_parameter`. It is a standalone template class `normaliser<ParameterClass>` that directly inherits from `mothernode`, `parameter_node_base`, and `no_processing`. It uses `SN_NODE_ID("normaliser")` (not `SN_POLY_NODE_ID`), confirming it is monophonic. It uses `SN_ADD_SET_VALUE` for the single "Value" parameter dispatch. There is no `PolyData` member -- no per-voice state.

## Parameters

- **Value**: Sole input parameter. Passed through unchanged. Uses `SN_ADD_SET_VALUE` macro for simple single-parameter dispatch.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
