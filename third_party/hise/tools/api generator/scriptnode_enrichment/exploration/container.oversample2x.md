# container.oversample2x - C++ Exploration (Variant)

**Base variant:** container.oversample
**Variant parameter:** OversampleFactor = 2 (fixed at compile time)

## Variant-Specific Behaviour

Uses `OversampleNode<2>` which wraps `wrap::oversample<2, DynamicSerialProcessor>`.
The oversampling factor is fixed at 2x and cannot be changed at runtime.

Differences from base variant:
- No Oversampling parameter exposed. Only FilterType (P=0) is available.
- No runtime factor switching -- no write lock contention from factor changes.
- Constructor passes 2 to `oversample_base`, setting `oversamplingFactor = jmax(1, 2) = 2`.
- Children always see `sampleRate * 2` and `blockSize * 2` (when not bypassed).
- `setParameter<0>` routes directly to `setFilterType()` (not `setOversamplingFactor()`).

All other behaviour is identical to the base variant: serial child dispatch,
JUCE dsp::Oversampling for up/downsample, same filter types (Polyphase/FIR),
same constraints (monophonic only, no frame processing, must be at original
sample rate), same bypass re-prepare behaviour.

Template instantiated at NodeContainerTypes.cpp:370.

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors:
  - parameter: FilterType, impact: minor, note: "FIR slightly more expensive than Polyphase."
  - note: "Fixed 2x factor means child processing cost is always doubled. At 44.1kHz, children process at 88.2kHz."
