# container.oversample8x - C++ Exploration (Variant)

**Base variant:** container.oversample
**Variant parameter:** OversampleFactor = 8 (fixed at compile time)

## Variant-Specific Behaviour

Uses `OversampleNode<8>` which wraps `wrap::oversample<8, DynamicSerialProcessor>`.
The oversampling factor is fixed at 8x and cannot be changed at runtime.

Differences from base variant:
- No Oversampling parameter exposed. Only FilterType (P=0) is available.
- No runtime factor switching -- no write lock contention from factor changes.
- Children always see `sampleRate * 8` and `blockSize * 8` (when not bypassed).
- At 44.1kHz, children process at 352.8kHz. Significant CPU impact.

All other behaviour identical to base variant.

Template instantiated at NodeContainerTypes.cpp:372.

## CPU Assessment

baseline: very_high
polyphonic: false
scalingFactors:
  - parameter: FilterType, impact: minor, note: "FIR slightly more expensive than Polyphase."
  - note: "Fixed 8x factor means child processing cost is multiplied by 8."
