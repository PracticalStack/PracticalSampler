# control.unscaler - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1166`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`, `pimpl::no_mod_normalisation`
**Classification:** control_source

## Signal Path

Value parameter input -> setValue() -> direct passthrough to output parameter (unnormalised).

unscaler is a simple passthrough node identical to normaliser but with unnormalised output. It forwards the raw Value without any transformation.

## Gap Answers

### passthrough-behaviour: Does unscaler simply pass the raw Value through?

Yes. The `setValue(double input)` method calls `this->getParameter().call(input)` with no transformation. The node inherits from `no_mod_normalisation(getStaticId(), { "Value" })`, which marks both the output as unnormalised and the Value input as unscaled. The `isNormalisedModulation()` method explicitly returns false.

### use-case-pattern: When would a user need this?

unscaler breaks out of the normalised 0..1 connection system. When a parameter source sends a value in a specific range (e.g., frequency in Hz from a converter), and the target parameter expects that exact value without range conversion, unscaler acts as the bridge. It is the unnormalised counterpart to `control.normaliser` (which passes through but with normalised output, meaning the target's range IS applied).

## Parameters

- **Value**: Raw input value, forwarded unchanged. Marked as unscaled input.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
