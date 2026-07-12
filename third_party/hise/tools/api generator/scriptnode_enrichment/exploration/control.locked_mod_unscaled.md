# control.locked_mod_unscaled - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1213`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`, `pimpl::no_mod_normalisation`
**Classification:** control_source

## Signal Path

Value parameter input -> setValue() -> direct passthrough to output parameter (unnormalised).

Identical to locked_mod except the output is unnormalised and the Value input is marked as unscaled.

## Gap Answers

### locked-container-interaction: Does locked_mod_unscaled use the same container discovery mechanism as locked_mod?

Yes. The C++ code is structurally identical to locked_mod. The only differences are: (1) inherits from `no_mod_normalisation`, (2) `isNormalisedModulation()` returns false, (3) Value is marked as unscaled via `no_mod_normalisation(getStaticId(), { "Value" })`. The container interaction is handled by the runtime infrastructure identically.

### unscaled-value-range: What range of values can flow through?

The Value input is marked as unscaled, meaning the connection system does not apply range conversion on incoming values. Any double value can flow through. The output is also unnormalised, so the raw value reaches the target parameter without range conversion. The range is effectively arbitrary, limited only by what the source sends.

## Parameters

- **Value**: Raw input value, forwarded unchanged. Marked as unscaled input.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
