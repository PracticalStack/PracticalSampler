# control.compare - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2215`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::compare>` (via alias at line 2740)
**Classification:** control_source

## Signal Path

Left and Right input parameters -> apply selected Comparator operation -> output 1.0 (true) or 0.0 (false) for comparison ops, or min/max value for MIN/MAX ops -> modulation output (normalised).

The `getValue()` method at line 2246:

```
switch (comparator)
{
    case Comparator::EQ:  return (double)(int)(leftValue == rightValue);
    case Comparator::NEQ: return (double)(int)(leftValue != rightValue);
    case Comparator::GT:  return (double)(int)(leftValue  > rightValue);
    case Comparator::LT:  return (double)(int)(leftValue  < rightValue);
    case Comparator::GET: return (double)(int)(leftValue >= rightValue);
    case Comparator::LET: return (double)(int)(leftValue <= rightValue);
    case Comparator::MIN: return jmin(leftValue, rightValue);
    case Comparator::MAX: return jmax(leftValue, rightValue);
}
```

## Gap Answers

### compare-operator-list: What are the 8 comparison operators?

The `Comparator` enum (line 2217) defines 8 operators:

| Index | Enum | Label | Operation |
|-------|------|-------|-----------|
| 0 | EQ | EQ | leftValue == rightValue |
| 1 | NEQ | NEQ | leftValue != rightValue |
| 2 | GT | GT | leftValue > rightValue |
| 3 | LT | LT | leftValue < rightValue |
| 4 | GET | GTE | leftValue >= rightValue |
| 5 | LET | LTE | leftValue <= rightValue |
| 6 | MIN | MIN | jmin(leftValue, rightValue) |
| 7 | MAX | MAX | jmax(leftValue, rightValue) |

Labels from `createParameters` at line 2307: `{ "EQ", "NEQ", "GT", "LT", "GTE", "LTE", "MIN", "MAX" }`. Note the label mismatch: enum values are GET/LET but UI labels are GTE/LTE.

### compare-output-values: Is the output strictly binary 0/1?

For comparators EQ through LET (indices 0-5): yes, strictly 1.0 or 0.0 via `(double)(int)(bool_expr)`. For MIN and MAX (indices 6-7): NOT binary -- returns `jmin` or `jmax` of the two input values, which can be any value in the 0..1 range. The comparison uses exact floating-point equality/comparison with no epsilon tolerance.

### compare-formula: What is the exact implementation?

As shown above. The comparison is direct floating-point comparison. No epsilon, no tolerance. For EQ, two doubles must be exactly equal to produce 1.0. MIN and MAX are notable outliers -- they return continuous values rather than binary, despite the node description saying "outputs either 1.0 or 0.0".

## Parameters

- **Left** (P=0): First comparison operand, range 0..1. Stored as `leftValue`. Change detection: only sets dirty if value actually changed (line 2270-2271).
- **Right** (P=1): Second comparison operand, range 0..1. Stored as `rightValue`. Same change detection (line 2276-2278).
- **Comparator** (P=2): Selects operation, range 0..7, integer step. Stored as enum. Always sets dirty on change (line 2284).

## Conditional Behaviour

The Comparator parameter selects between 8 operations. Six produce binary output (0/1), two produce continuous output (MIN/MAX). The dirty flag management for Left/Right only triggers output when the value actually changes (line 2271: `dirty |= leftValue != prevValue`).

## Polyphonic Behaviour

Uses `PolyData<multilogic::compare, NV>`. Each voice has independent leftValue, rightValue, comparator, and dirty flag. The `reset()` method (line 2242) is empty -- no state reset on voice start.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

MIN and MAX modes return continuous values, not binary. This contradicts the node description "outputs either 1.0 or 0.0". This is a documentation inaccuracy, not a bug -- MIN/MAX are useful continuous operations.
