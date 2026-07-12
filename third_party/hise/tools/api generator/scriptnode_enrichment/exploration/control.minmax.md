# control.minmax - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2451`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::minmax>` (via alias at line 2738)
**Classification:** control_source

## Signal Path

Value parameter (0..1) -> apply InvertableParameterRange::convertFrom0to1() using Minimum/Maximum/Skew/Polarity -> snap to Step interval -> modulation output (unnormalised).

The node uses `multilogic::minmax` which inherits from `pimpl::no_mod_normalisation`. The `getValue()` method at line 2464:

```
auto v = range.convertFrom0to1(value, true);
v = range.snapToLegalValue(v);
return v;
```

The `convertFrom0to1` call applies the full NormalisableRange conversion (min, max, skew) with inversion support. `snapToLegalValue` quantises to the interval if Step > 0.

## Gap Answers

### minmax-mapping-formula: What is the exact mapping formula?

It uses `InvertableParameterRange::convertFrom0to1(value, true)` which wraps `juce::NormalisableRange<double>::convertFrom0to1()` with inversion support. The second argument `true` means inversion IS applied when the `inv` flag is set. The range is constructed from the Minimum, Maximum, Skew, and Step parameters. After conversion, `snapToLegalValue(v)` quantises the result to the interval (Step) if it is non-zero.

### minmax-polarity-behaviour: What does the Polarity parameter do exactly?

Polarity maps to `range.inv` (line 2485): `range.inv = v > 0.5`. When Polarity is 1 (Inverted), the `inv` flag is set to true. The `convertFrom0to1(value, true)` method in `InvertableParameterRange` applies inversion by computing `1.0 - value` before the range mapping. So Polarity inverts the INPUT before range conversion. With Polarity=Normal and Value=0, output is Minimum. With Polarity=Inverted and Value=0, output is Maximum. The `createParameters` call at line 2531 confirms labels: `{ "Normal", "Inverted" }`.

### minmax-step-quantisation: How does the Step parameter interact with the range mapping?

Step is stored as `range.rng.interval` (line 2483). After `convertFrom0to1()` produces a value in the [Minimum, Maximum] range, `snapToLegalValue(v)` quantises it to the nearest multiple of the interval. With Step=0, no quantisation occurs. With Step=0.1, output values snap to 0.0, 0.1, 0.2, etc.

### minmax-unnormalised-input-note: Value input IS scaled (0..1) but the output is unnormalised?

Confirmed. The `no_mod_normalisation` constructor at line 2457 passes an empty `{}` list for unscaled input parameters. This means NO input parameters are marked as unscaled -- Value receives standard normalised 0..1 input. But the node itself inherits `no_mod_normalisation` which sets `UseUnnormalisedModulation` and returns `isNormalisedModulation() == false`, so the OUTPUT is unnormalised. This asymmetry (normalised 0..1 input, unnormalised output) is the defining characteristic of minmax.

## Parameters

- **Value** (P=0): Normalised 0..1 input to be remapped. Stored as `value`.
- **Minimum** (P=1): Output range minimum. Stored as `range.rng.start`.
- **Maximum** (P=2): Output range maximum. Stored as `range.rng.end`.
- **Skew** (P=3): Skew factor for non-linear mapping, clamped to 0.1..10.0. Stored as `range.rng.skew`.
- **Step** (P=4): Quantisation interval. Stored as `range.rng.interval`. 0 = no snapping.
- **Polarity** (P=5): Boolean toggle (Normal/Inverted). Sets `range.inv`. Inverts the input before range mapping.

After any parameter change, `range.checkIfIdentity()` is called (line 2487) to optimise the identity case.

## Polyphonic Behaviour

Uses `PolyData<multilogic::minmax, NV>`. Each voice has independent value, range, and dirty flag.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
