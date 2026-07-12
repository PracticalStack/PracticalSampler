# control.random - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1291`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

Value parameter changes -> setValue() -> generates random value -> forwards to output parameter.

## Gap Answers

### random-trigger: When does the random value get generated?

On every `setValue()` call, regardless of the input value. The input parameter value is completely ignored (the `double` argument is unnamed). Every time any value arrives at the Value parameter, a new random number is generated and sent. There is no edge detection or threshold.

### random-distribution: What random distribution is used?

Uses `r.nextDouble()` where `r` is a JUCE `Random` object (member variable). This produces a uniform distribution in [0.0, 1.0). The Random object uses JUCE's default PRNG (a linear congruential generator seeded from system time at construction).

## Parameters

- **Value**: Trigger input. The actual value is ignored; any change triggers a new random output.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The output is normalised (no `no_mod_normalisation` inheritance), so target parameter ranges are applied by the connection system. The Random object is default-constructed, meaning each instance gets a different seed based on system time.
