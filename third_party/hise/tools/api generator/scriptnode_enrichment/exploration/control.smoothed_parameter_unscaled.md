# control.smoothed_parameter_unscaled - C++ Exploration (Variant)

**Base variant:** control.smoothed_parameter
**Variant parameter:** `IsScaled = false` (unnormalised output, unscaled Value input)

## Variant-Specific Behaviour

smoothed_parameter_unscaled is the same `smoothed_parameter_pimpl` template with `IsScaled = false`. The only differences:

1. `isNormalisedModulation()` returns false (output is unnormalised).
2. Registers `UseUnnormalisedModulation` property and marks "Value" as unscaled input via `addUnscaledParameter`.
3. Static ID returns `"smoothed_parameter_unscaled"` instead of `"smoothed_parameter"`.

The smoothing algorithm, mode options, parameters, and processing pattern are identical to smoothed_parameter.

## Gap Answers

### unscaled-difference: Is the only difference the normalisation?

Yes. Both variants use the same `smoothed_parameter_pimpl` template class. The `IsScaled` template boolean controls only normalisation behaviour and property registration. All smoothing logic, mode selection, and parameter handling are shared.

### smoothing-modes: Same modes as smoothed_parameter?

Yes. The mode namespace `"smoothers"` is shared. Available modes: linear_ramp, low_pass, no. Identical behaviour.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []
