# control.blend - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2050` (multilogic::blend) + line 2745 (type alias)
**Base class:** `multi_parameter<NV, ParameterType, multilogic::blend>`
**Classification:** control_source

## Signal Path

The blend node linearly interpolates between two input values (Value1, Value2) based on the Alpha parameter. The output is `Interpolator::interpolateLinear(value1, value2, alpha)`, which computes `value1 + (value2 - value1) * alpha`.

Alpha parameter -> blend ratio
Value1, Value2 -> two endpoints
Output = value1 + (value2 - value1) * alpha

## Gap Answers

### blend-formula: What is the exact blending formula?

Standard linear interpolation. `getValue()` (line 2063-2067) returns `Interpolator::interpolateLinear(value1, value2, alpha)`. This is the standard formula: `output = value1 + (value2 - value1) * alpha`. When Alpha=0, output = Value1. When Alpha=1, output = Value2. Intermediate values are linear.

### blend-normalisation: Is the output normalised or unnormalised?

Unnormalised. `multilogic::blend::isNormalisedModulation()` returns `false` (line 2055). This confirms the control-infrastructure doc is correct. The preliminary JSON's `modulationOutput.isUnnormalised: false` is INCORRECT -- it should be `true`. The blend node inherits unnormalised behavior from the multilogic class, not from `no_mod_normalisation` base (blend does not inherit from `no_mod_normalisation`), but the `isNormalisedModulation()` method directly returns false on the DataType, and `multi_parameter::isNormalisedModulation()` delegates to `DataType::isNormalisedModulation()` (line 2650).

### blend-output-clamping: Is the blended output clamped?

No clamping. `getValue()` returns the raw interpolation result without any `jlimit` call. If Value1 and Value2 are within 0..1, the output stays in 0..1 (linear interpolation property). If either value is outside 0..1 (possible since the parameters have no enforcement), the output can exceed that range.

## Parameters

- **Alpha** (P=0): Blend ratio. Range 0..1, default 0.0. Alpha=0 selects Value1, Alpha=1 selects Value2.
- **Value1** (P=1): First input value. Default 0.0. No range set in createParameters (uses default).
- **Value2** (P=2): Second input value. Default 0.0. No range set in createParameters (uses default).

Note: The `createParameters()` (line 2081-2101) does NOT set any range for Value1 or Value2, and does NOT mark them as unscaled. The blend multilogic class does NOT inherit from `no_mod_normalisation`, so no input parameters are registered as unscaled.

## Polyphonic Behaviour

Uses `PolyData<multilogic::blend, NV>`. Each voice has independent alpha, value1, value2, and dirty state.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The preliminary JSON states `modulationOutput.isUnnormalised: false` which contradicts the C++ source where `isNormalisedModulation()` returns false (meaning output IS unnormalised). This is flagged in issues.md.
