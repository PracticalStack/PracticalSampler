# jdsp.jdelay_cubic -- C++ Exploration (Variant)

**Base variant:** jdsp.jdelay
**Variant parameter:** Interpolation type = Lagrange3rd (cubic)

## Variant-Specific Behaviour

Uses `CubicDelay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>`.

All wrapper logic, parameter handling, deferred initialisation, polyphonic range reduction (0-30ms), and signal path are identical to jdelay. The only difference is the interpolation algorithm used when reading from the delay buffer at non-integer sample positions.

Lagrange 3rd-order interpolation uses a four-point polynomial fit. This provides:
- Flat amplitude response across the audio range (no high-frequency roll-off)
- Suitable for modulated delay effects where tonal colouring is undesirable
- Highest CPU cost of the three variants (four multiply-adds per sample per channel vs one for linear)

The description says "modulatable" -- this means rapid delay time changes produce cleaner results than Thiran because cubic Lagrange has no internal filter state that needs to settle.

## CPU Assessment

baseline: medium
polyphonic: true
scalingFactors:
  - parameter: "Limit", impact: "memory", note: "Larger Limit allocates more buffer memory but does not increase CPU"
