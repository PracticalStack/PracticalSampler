# container.oversample4x - C++ Exploration (Variant)

**Base variant:** container.oversample
**Variant parameter:** OversampleFactor = 4 (fixed at compile time)

## Variant-Specific Behaviour

Uses `OversampleNode<4>` which wraps `wrap::oversample<4, DynamicSerialProcessor>`.
The oversampling factor is fixed at 4x and cannot be changed at runtime.

Differences from base variant:
- No Oversampling parameter exposed. Only FilterType (P=0) is available.
- No runtime factor switching -- no write lock contention from factor changes.
- Children always see `sampleRate * 4` and `blockSize * 4` (when not bypassed).
- At 44.1kHz, children process at 176.4kHz.

All other behaviour identical to base variant.

Template instantiated at NodeContainerTypes.cpp:371.

## CPU Assessment

baseline: high
polyphonic: false
scalingFactors:
  - parameter: FilterType, impact: minor, note: "FIR slightly more expensive than Polyphase."
  - note: "Fixed 4x factor means child processing cost is always quadrupled."
