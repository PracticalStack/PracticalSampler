# container.fix128_block -- C++ Exploration (Variant)

**Base variant:** container.fix8_block
**Variant parameter:** BlockSize = 128

## Variant-Specific Behaviour

Identical to fix8_block except `wrap::fix_block<128, T>`. A 512-sample host
buffer produces 4 chunks of 128. Children see blockSize=128 in PrepareSpecs.

Explicit template instantiation at NodeContainerTypes.cpp:483.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
