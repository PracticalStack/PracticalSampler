---
title: fix32_block
description: "Splits the audio buffer into chunks of 32 samples for higher modulation update rates."
factoryPath: container.fix32_block
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
  container.fix32_block

  Serial container that splits the incoming audio buffer into chunks of at most 32 samples. Children process each chunk sequentially. Increases modulation update rate to roughly 1.4 kHz at 44.1 kHz sample rate.

  Signal flow:
    audio in -> split into 32-sample chunks -> children process each chunk serially -> audio out

  CPU: negligible, monophonic

  Parameters: none

  When to use:
    Good general-purpose choice for modulation precision. Handles most filter and pitch modulation without audible stepping while keeping chunk overhead low (16 iterations for a 512-sample buffer).

  Key details:
    In send/receive feedback loops, routing delay equals one processing chunk (32 samples). Use frame processing to reduce to 1 sample.

  See also:
    [alternative] container.fix_blockx - adjustable block size via property
    [disambiguation] container.frame2_block - per-sample processing instead of block chunking
---

The `fix32_block` container splits the incoming audio buffer into chunks of at most 32 samples and processes its children serially on each chunk. This provides a modulation update rate of roughly 1.4 kHz at 44.1 kHz sample rate with moderate chunk iteration overhead (16 iterations for a 512-sample buffer).

This is a good general-purpose choice when modulation precision matters. A 32-sample update interval is short enough to handle most filter cutoff and pitch modulation without audible stepping, while keeping per-chunk overhead well below what [fix8_block]($SN.container.fix8_block$) or [fix16_block]($SN.container.fix16_block$) would add. If unsure which block size to choose, consider using [fix_blockx]($SN.container.fix_blockx$) to evaluate different sizes during development.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.process:
      desc: "Processes all child nodes serially on the current chunk"
---

```
// container.fix32_block - splits buffer into 32-sample chunks
// audio in -> audio out

process(input) {
    for each chunk of 32 samples in input:
        children.process(chunk)
}
```

::

The block size of 32 is a maximum -- the last chunk may be smaller if the host buffer size is not a multiple of 32. MIDI events are distributed across chunks by timestamp for sub-block timing accuracy.

### Feedback Loops

In feedback loops using send/receive nodes, the routing delay is exactly one processing chunk. With a 32-sample block size, this means 32 samples of latency in the feedback path. To minimise feedback delay, use a frame-based container instead, which reduces the routing delay to a single sample.

When bypassed, children process the full host buffer without chunking, equivalent to a [container.chain]($SN.container.chain$). Toggling bypass triggers a full re-preparation of all children. If the network is already in frame mode, this container becomes a no-op.

**See also:** $SN.container.fix_blockx$ -- adjustable block size via property, $SN.container.frame2_block$ -- per-sample processing instead of block chunking
