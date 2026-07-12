# jdsp.jdelay_thiran -- C++ Exploration (Variant)

**Base variant:** jdsp.jdelay
**Variant parameter:** Interpolation type = Thiran (allpass)

## Variant-Specific Behaviour

Uses `ThiranDelay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>`.

All wrapper logic, parameter handling, deferred initialisation, polyphonic range reduction (0-30ms), and signal path are identical to jdelay. The only difference is the interpolation algorithm.

Thiran allpass interpolation uses a first-order allpass filter to achieve fractional sample delay. This provides:
- Flat amplitude response (allpass preserves magnitude)
- Good performance -- comparable to linear interpolation (one allpass coefficient calculation)
- NOT suitable for fast modulation: the allpass filter has internal state. Rapid changes to the delay time cause the filter coefficients to change abruptly, producing transient artefacts. The filter needs time to settle after each coefficient change.

The description's warning about fast modulation is accurate. For modulated effects (chorus, flanger), prefer jdelay (cheaper, acceptable colouring) or jdelay_cubic (cleaner, more expensive).

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors:
  - parameter: "Limit", impact: "memory", note: "Larger Limit allocates more buffer memory but does not increase CPU"
