---
title: fix64_block
description: "Splits the audio buffer into chunks of 64 samples for higher modulation update rates."
factoryPath: container.fix64_block
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
  container.fix64_block

  Serial container that splits the incoming audio buffer into chunks of at most 64 samples. Children process each chunk sequentially. Increases modulation update rate to roughly 689 Hz at 44.1 kHz sample rate.

  Signal flow:
    audio in -> split into 64-sample chunks -> children process each chunk serially -> audio out

  CPU: negligible, monophonic

  Parameters: none

  When to use:
    Default block size used by fix_blockx and dynamic_blocksize. Suitable for moderate modulation precision with minimal overhead (8 iterations for a 512-sample buffer). Good starting point when evaluating block size tradeoffs.

  See also:
    [alternative] container.fix_blockx - adjustable block size via property
    [disambiguation] container.frame2_block - per-sample processing instead of block chunking
---

The `fix64_block` container splits the incoming audio buffer into chunks of at most 64 samples and processes its children serially on each chunk. This provides a modulation update rate of roughly 689 Hz at 44.1 kHz sample rate with minimal overhead (8 iterations for a 512-sample buffer).

64 samples is the default block size used by both [fix_blockx]($SN.container.fix_blockx$) and [dynamic_blocksize]($SN.container.dynamic_blocksize$). It is a good starting point when you need some modulation precision improvement but want to keep CPU overhead low. For control-rate modulation such as LFO-driven parameter changes, this resolution is typically sufficient. For audio-rate modulation of filter cutoff or pitch, consider [fix32_block]($SN.container.fix32_block$) or smaller.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.process:
      desc: "Processes all child nodes serially on the current chunk"
---

```
// container.fix64_block - splits buffer into 64-sample chunks
// audio in -> audio out

process(input) {
    for each chunk of 64 samples in input:
        children.process(chunk)
}
```

::

The block size of 64 is a maximum -- the last chunk may be smaller if the host buffer size is not a multiple of 64. MIDI events are distributed across chunks by timestamp for sub-block timing accuracy. When bypassed, children process the full host buffer without chunking, equivalent to a [container.chain]($SN.container.chain$). Toggling bypass triggers a full re-preparation of all children. If the network is already in frame mode, this container becomes a no-op.

**See also:** $SN.container.fix_blockx$ -- adjustable block size via property, $SN.container.frame2_block$ -- per-sample processing instead of block chunking
