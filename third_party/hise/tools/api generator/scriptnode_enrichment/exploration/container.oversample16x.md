# container.oversample16x - C++ Exploration (Variant)

**Base variant:** container.oversample
**Variant parameter:** OversampleFactor = 16 (fixed at compile time)

## Variant-Specific Behaviour

Uses `OversampleNode<16>` which wraps `wrap::oversample<16, DynamicSerialProcessor>`.
The oversampling factor is fixed at 16x and cannot be changed at runtime. This is
the maximum oversampling factor (MaxOversamplingExponent = 4, 2^4 = 16).

Differences from base variant:
- No Oversampling parameter exposed. Only FilterType (P=0) is available.
- No runtime factor switching -- no write lock contention from factor changes.
- Children always see `sampleRate * 16` and `blockSize * 16` (when not bypassed).
- At 44.1kHz, children process at 705.6kHz. Extreme CPU impact.

All other behaviour identical to base variant.

Template instantiated at NodeContainerTypes.cpp:373.

## CPU Assessment

baseline: very_high
polyphonic: false
scalingFactors:
  - parameter: FilterType, impact: minor, note: "FIR slightly more expensive than Polyphase."
  - note: "Fixed 16x factor means child processing cost is multiplied by 16. Use only when necessary (e.g., aggressive waveshaping with harmonics near Nyquist)."
