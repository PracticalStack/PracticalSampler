# container.fix16_block -- C++ Exploration (Variant)

**Base variant:** container.fix8_block
**Variant parameter:** BlockSize = 16

## Variant-Specific Behaviour

Identical to fix8_block in all respects except `wrap::fix_block<16, T>` is used
instead of `wrap::fix_block<8, T>`. The template parameter changes only the chunk
size passed to `ChunkableProcessData`.

A 512-sample host buffer produces 32 chunks of 16 (vs. 64 chunks of 8).
Children see blockSize=16 in PrepareSpecs (or min(16, hostBlockSize) if host
buffer is smaller).

Explicit template instantiation at NodeContainerTypes.cpp:480.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

Half the chunk iterations of fix8_block for the same host buffer size.
