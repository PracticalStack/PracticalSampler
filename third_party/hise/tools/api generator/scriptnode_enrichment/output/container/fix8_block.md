---
title: fix8_block
description: "Splits the audio buffer into chunks of 8 samples for higher modulation update rates."
factoryPath: container.fix8_block
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
commonMistakes:
  - title: "Wrapping nodes that do not benefit"
    wrong: "Placing the entire network inside a fix8_block"
    right: "Wrap only the section where modulation precision matters."
    explanation: "Block subdivision adds per-chunk call overhead. Nodes that are not modulated gain nothing from a smaller block size. Keep unmodulated processing outside the container."
llmRef: |
  container.fix8_block

  Serial container that splits the incoming audio buffer into chunks of at most 8 samples. Children process each chunk sequentially. Increases modulation update rate from once per host buffer to once per 8 samples.

  Signal flow:
    audio in -> split into 8-sample chunks -> children process each chunk serially -> audio out

  CPU: negligible, monophonic

  Parameters: none

  When to use:
    When modulation of child node parameters needs a faster update rate than the host buffer allows. Provides the highest precision of the fixN_block family at the cost of more chunk iterations. For true per-sample accuracy, use a frame container instead.

  See also:
    [alternative] container.fix_blockx - adjustable block size via property
    [disambiguation] container.frame2_block - per-sample processing instead of block chunking
---

The `fix8_block` container splits the incoming audio buffer into chunks of at most 8 samples and processes its children serially on each chunk. This increases the modulation update rate: parameters modulated by control signals are updated once per chunk rather than once per host buffer, giving roughly 5.5 kHz update rate at 44.1 kHz sample rate.

This is the smallest fixed block size available. It provides the highest modulation precision of the `fixN_block` family but produces the most chunk iterations per buffer (64 iterations for a 512-sample buffer). For most modulated parameters, [fix32_block]($SN.container.fix32_block$) or [fix64_block]($SN.container.fix64_block$) offer a good balance between precision and CPU overhead. Reserve this node for cases where the modulated parameter is highly audible at lower update rates.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.process:
      desc: "Processes all child nodes serially on the current chunk"
---

```
// container.fix8_block - splits buffer into 8-sample chunks
// audio in -> audio out

process(input) {
    for each chunk of 8 samples in input:
        children.process(chunk)
}
```

::

The block size of 8 is a maximum, not a fixed value -- the last chunk in a buffer may be smaller if the host buffer size is not a multiple of 8. MIDI events are distributed across chunks by timestamp, providing sub-block timing accuracy.

### Bypass Behaviour

When bypassed, children process the full host buffer without chunking, equivalent to a [container.chain]($SN.container.chain$). This allows A/B comparison of the block subdivision effect. Toggling bypass triggers a full re-preparation of all children with the original block size. If the network is already in frame mode (block size of 1), this container becomes a no-op and frame mode is preserved through to children.

**See also:** $SN.container.fix_blockx$ -- adjustable block size via property, $SN.container.frame2_block$ -- per-sample processing instead of block chunking
