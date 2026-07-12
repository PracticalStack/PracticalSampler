# container.fix32_block -- C++ Exploration (Variant)

**Base variant:** container.fix8_block
**Variant parameter:** BlockSize = 32

## Variant-Specific Behaviour

Identical to fix8_block except `wrap::fix_block<32, T>`. A 512-sample host
buffer produces 16 chunks of 32. Children see blockSize=32 in PrepareSpecs.

Explicit template instantiation at NodeContainerTypes.cpp:481.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
