# container.fix256_block -- C++ Exploration (Variant)

**Base variant:** container.fix8_block
**Variant parameter:** BlockSize = 256

## Variant-Specific Behaviour

Identical to fix8_block except `wrap::fix_block<256, T>`. A 512-sample host
buffer produces 2 chunks of 256. Children see blockSize=256 in PrepareSpecs.

Explicit template instantiation at NodeContainerTypes.cpp:484.

Note: This is the largest fixN_block variant with explicit template instantiation.
BlockSize=512 is only available through fix_blockx and dynamic_blocksize.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
