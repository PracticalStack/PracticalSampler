# control.change - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2319`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::change>` (via alias at line 2744)
**Classification:** control_source

## Signal Path

Value parameter (raw, unscaled) -> change detection (exact float equality) -> if changed: forward value to modulation output (unnormalised); if same: suppress.

The `multilogic::change` struct at line 2319. Its `setParameter<0>()` at line 2338:

```
dirty = v != value;
value = v;
```

And `getValue()` at line 2332:

```
dirty = false;
return value;
```

The dirty flag is only set when the new value differs from the stored value (exact `!=` comparison). The `multi_parameter::sendPending()` only calls `getParameter().call()` when dirty is true. So duplicate values are filtered out.

## Gap Answers

### change-comparison-method: How is 'same value' determined?

Exact floating-point inequality: `v != value` (line 2342). There is no epsilon or tolerance. The comparison uses standard IEEE 754 double-precision equality. This means values that differ by even the smallest representable amount will pass through, while only bit-identical values are filtered.

### change-initial-value: What happens on the first value received?

The member default is `value = 0.0` (line 2358). On the first call to `setParameter<0>()`, the comparison `v != value` checks against 0.0. If the first value is 0.0, dirty remains false and no output is sent. If the first value is anything other than 0.0, dirty becomes true and the value passes through. There is no special "first value always passes" logic.

### change-unscaled-passthrough: Does change simply pass through the raw value when it changes?

Yes. The `getValue()` method simply returns `value` without any transformation. The node is purely a gate/filter -- it suppresses duplicate values but does not modify the value itself. Both input and output are unnormalised (inherits `no_mod_normalisation` with `{ "Value" }` as unscaled input).

## Parameters

- **Value** (P=0): Sole input. Unscaled (receives raw values). Stored as `value`. Change detection applied on assignment.

## Polyphonic Behaviour

Uses `PolyData<multilogic::change, NV>`. Each voice has independent value and dirty flag. Per-voice change detection means a value might be "new" for one voice but "same" for another.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
