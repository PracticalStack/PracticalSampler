# routing.local_cable_unscaled - C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/dynamic_elements/DynamicRoutingNodes.h:710`
**Base class:** `local_cable_base` + `control::pimpl::no_mod_normalisation`
**Classification:** control_source

## Signal Path

Identical to `routing.local_cable` -- routes a control value between nodes sharing the same LocalId within a DspNetwork. The key difference is that values pass through without normalisation on both input and output.

The `setValue()` method is inherited from `local_cable_base` -- same broadcast logic with recursion guard.

## Gap Answers

### empty-description: The base data description is empty. What is an accurate description distinguishing this from routing.local_cable?

Based on the C++ source: "Routes an unnormalised control value between nodes sharing the same LocalId within a single DspNetwork. Values bypass range conversion on both input and output."

### unscaled-difference: UseUnnormalisedModulation lists 'Value' as an unscaled parameter. Does this mean the Value input bypasses range conversion AND the modulation output sends raw values? Or just the output?

Both directions are unscaled. The constructor calls `no_mod_normalisation(getStaticId(), { "Value" })` which:
1. Registers `UseUnnormalisedModulation` property on the node -- marking the modulation OUTPUT as unnormalised (`isNormalisedModulation()` returns `false`)
2. Calls `addUnscaledParameter(nodeId, "Value")` -- marking the Value INPUT parameter as unscaled

So incoming values to the Value parameter bypass range conversion, and the modulation output sends raw values without normalisation to 0..1. This matches the `no_mod_normalisation` contract described in the control infrastructure.

### value-range-in-practice: The parameter range shows 0-1, but with unnormalised modulation the actual values passed could be outside 0-1. What is the effective value range in practice?

The effective range is arbitrary -- whatever the source sends. The 0-1 range in the parameter definition is a nominal default, but because both input and output are marked as unscaled, the system does not clamp or convert values through this range. If an unnormalised source sends 440.0, that value passes through unchanged to any connected targets. The practical range depends entirely on the connected source and what it outputs.

## Parameters

- **Value** (nominal 0..1, effectively arbitrary): The control value routed through the cable. Unscaled on both input and output -- values pass through without range conversion.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

- The ONLY code difference from `local_cable` is the additional `no_mod_normalisation` base class in the constructor with `{ "Value" }` as the unscaled parameter list, and the override `static constexpr bool isNormalisedModulation() { return false; }`.
- All signal flow logic, recursion guard, Manager interaction, and broadcasting are identical to `local_cable`.
