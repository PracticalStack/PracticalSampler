# control.pma_unscaled - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2593`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::pma_unscaled>` (via alias at line 2736)
**Classification:** control_source

## Signal Path

Value parameter (raw) -> multiply by Multiply -> add Add (raw) -> modulation output (unnormalised, no clamping).

The node uses `multilogic::pma_unscaled` which inherits from `pma_base` (same setParameter/createParameters) but overrides `getValue()` at line 2606:

```
return value * mulValue + addValue;
```

No clamping is applied. Output is unnormalised -- target parameter ranges are NOT applied by the connection system.

## Gap Answers

### pma-unscaled-formula: In multilogic::pma_unscaled::getValue(), is the formula value * mulValue + addValue WITHOUT clamping?

Confirmed. Line 2609: `return value * mulValue + addValue;` -- no `jlimit` call, unlike the normalised `pma` variant. The output is truly unclamped.

### pma-unscaled-range-passthrough: Since both Value and Add are marked as unscaled inputs, do they receive raw values from connected sources? What happens if Multiply is connected?

The constructor at line 2602 passes `{ "Value", "Add" }` to `no_mod_normalisation`, marking both as unscaled input parameters. This means Value and Add receive raw values without range conversion from their sources. Multiply is NOT in the unscaled list, so if Multiply is connected to a modulation source, it receives a range-scaled value (the source's normalised 0..1 output is converted through Multiply's -1..1 range).

### pma-unscaled-output-range: What is the practical output range? Is there any overflow protection?

There is no overflow protection. The output range is entirely determined by the connected sources. Since `getValue()` returns `value * mulValue + addValue` without clamping, the output can be any double value. If Value is a frequency in Hz (e.g. 440) and Multiply is 1.0 and Add is 0, the output is 440.0. There are no guards against infinity, NaN, or extreme values.

## Parameters

- **Value** (P=0): Primary input, unscaled. Range in JSON is 0..1 but receives raw values. Stored as `value`.
- **Multiply** (P=1): Scale factor, range -1..1, default 1.0. Stored as `mulValue`. NOT unscaled.
- **Add** (P=2): DC offset, unscaled. Range in JSON is -1..1 but receives raw values. Stored as `addValue`.

## Polyphonic Behaviour

Same as control.pma -- uses `PolyData<multilogic::pma_unscaled, NV>` with per-voice state.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
