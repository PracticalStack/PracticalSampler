---
title: fix256_block
description: "Splits the audio buffer into chunks of 256 samples for higher modulation update rates."
factoryPath: container.fix256_block
factory: container
polyphonic: false
tags: [container, block-size, modulation-precision]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.fix_blockx", type: alternative, reason: "Adjustable block size via property for evaluating different chunk sizes" }
  - { id: "container.frame2_block", type: disambiguation, reason: "Per-sample processing instead of block chunking" }
llmRef: |
  container.fix256_block

  Serial container that splits the incoming audio buffer into chunks of at most 256 samples. Children process each chunk sequentially. Increases modulation update rate to roughly 172 Hz at 44.1 kHz sample rate.

  Signal flow:
    audio in -> split into 256-sample chunks -> children process each chunk serially -> audio out

  CPU: negligible, monophonic

  Parameters: none

  When to use:
    Useful only with very large host buffers (512+) where even a single subdivision improves modulation timing. The largest fixN_block variant. For block size 512, use fix_blockx or dynamic_blocksize instead.

  See also:
    [alternative] container.fix_blockx - adjustable block size via property
    [disambiguation] container.frame2_block - per-sample processing instead of block chunking
---

The `fix256_block` container splits the incoming audio buffer into chunks of at most 256 samples and processes its children serially on each chunk. This provides a modulation update rate of roughly 172 Hz at 44.1 kHz sample rate.

This is the largest fixed block size variant. It only produces meaningful subdivision when the host buffer is 512 samples or larger (2 iterations for a 512-sample buffer). With a 256-sample host buffer, no chunking occurs at all since the buffer already fits within the limit. This node is primarily useful when the host runs at very large buffer sizes and even a basic subdivision improves modulation smoothness. For finer control or block sizes above 256, use [fix_blockx]($SN.container.fix_blockx$) or [dynamic_blocksize]($SN.container.dynamic_blocksize$) which support up to 512.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.process:
      desc: "Processes all child nodes serially on the current chunk"
---

```
// container.fix256_block - splits buffer into 256-sample chunks
// audio in -> audio out

process(input) {
    for each chunk of 256 samples in input:
        children.process(chunk)
}
```

::

The block size of 256 is a maximum -- the last chunk may be smaller if the host buffer size is not a multiple of 256. MIDI events are distributed across chunks by timestamp for sub-block timing accuracy. When bypassed, children process the full host buffer without chunking, equivalent to a [container.chain]($SN.container.chain$). Toggling bypass triggers a full re-preparation of all children. If the network is already in frame mode, this container becomes a no-op.

**See also:** $SN.container.fix_blockx$ -- adjustable block size via property, $SN.container.frame2_block$ -- per-sample processing instead of block chunking
