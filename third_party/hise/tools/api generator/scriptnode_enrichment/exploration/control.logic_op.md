# control.logic_op - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2110`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::logic_op>` (via alias at line 2739)
**Classification:** control_source

## Signal Path

Left and Right input parameters -> convert to boolean (> 0.5 threshold) -> apply selected logic operator (AND/OR/XOR) -> output 1.0 (true) or 0.0 (false) -> modulation output (normalised).

The `getValue()` method at line 2145:

```
auto lv = leftValue == LogicState::True;
auto rv = rightValue == LogicState::True;
switch (logicType)
{
    case LogicType::AND: return lv && rv ? 1.0 : 0.0;
    case LogicType::OR:  return lv || rv ? 1.0 : 0.0;
    case LogicType::XOR: return (lv || rv) && !(lv == rv) ? 1.0 : 0.0;
}
```

## Gap Answers

### logic-op-operator-list: What are the 3 logic operators?

The `LogicType` enum (line 2119) defines 3 operators:

| Index | Enum | Label | Truth Table |
|-------|------|-------|-------------|
| 0 | AND | AND | 1 only when both inputs are true |
| 1 | OR | OR | 1 when either or both inputs are true |
| 2 | XOR | XOR | 1 when exactly one input is true |

Labels from `createParameters` at line 2203: `{ "AND", "OR", "XOR" }`.

### logic-op-threshold: How are inputs converted to boolean?

The threshold is `> 0.5`. In `setParameter<0>()` (line 2163): `leftValue = LogicState(int(v > 0.5) + 1)`. Since `LogicState` has values Undefined=0, False=1, True=2, the expression `int(v > 0.5) + 1` produces False(1) when v <= 0.5 and True(2) when v > 0.5. The same applies to Right (line 2171).

### logic-op-output-values: Does the output strictly produce 1.0 or 0.0?

Yes. All three operators use ternary expressions returning `1.0 : 0.0`. The output is strictly binary. `isNormalisedModulation()` returns true.

## Parameters

- **Left** (P=0): First logic input, range 0..1. Converted to boolean via > 0.5 threshold. Stored as `LogicState` enum.
- **Right** (P=1): Second logic input, range 0..1. Same threshold conversion. Stored as `LogicState` enum.
- **Operator** (P=2): Selects logic operation, range 0..2, integer step. Stored as `LogicType` enum.

## Conditional Behaviour

The Operator parameter selects between AND, OR, and XOR. The dirty flag has sophisticated gating: Left/Right changes only set dirty when (a) the LogicState actually changes AND (b) the other input has been defined (not Undefined). This prevents spurious output before both inputs have been set. The Operator change always sets dirty.

The `reset()` method (line 2138) sets both inputs to `LogicState::Undefined` and dirty to false, preventing output until both inputs are explicitly set after a voice reset.

## Polyphonic Behaviour

Uses `PolyData<multilogic::logic_op, NV>`. Each voice has independent leftValue, rightValue, logicType, and dirty flag. The `reset()` method clears state to Undefined on voice start.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
