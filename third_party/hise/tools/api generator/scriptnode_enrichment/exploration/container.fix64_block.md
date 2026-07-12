# container.fix64_block -- C++ Exploration (Variant)

**Base variant:** container.fix8_block
**Variant parameter:** BlockSize = 64

## Variant-Specific Behaviour

Identical to fix8_block except `wrap::fix_block<64, T>`. A 512-sample host
buffer produces 8 chunks of 64. Children see blockSize=64 in PrepareSpecs.

This is the default block size used by fix_blockx and dynamic_blocksize when
no explicit value is configured.

Explicit template instantiation at NodeContainerTypes.cpp:482.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
