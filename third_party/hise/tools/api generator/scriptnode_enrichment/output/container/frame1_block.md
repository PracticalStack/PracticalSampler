---
title: frame1_block
description: "Enables per-sample processing for child nodes on a single mono channel."
factoryPath: container.frame1_block
factory: container
polyphonic: false
tags: [container, frame, per-sample, mono]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.frame2_block", type: disambiguation, reason: "Stereo per-sample processing (processes both channels)" }
  - { id: "container.framex_block", type: alternative, reason: "Dynamic channel count per-sample processing" }
  - { id: "container.fix8_block", type: disambiguation, reason: "Block chunking as a lower-cost alternative to per-sample processing" }
commonMistakes:
  - title: "Using in a stereo network expecting both channels"
    wrong: "Placing frame1_block in a stereo network and expecting both channels to be processed"
    right: "Use frame2_block for stereo networks. frame1_block only processes channel 0; other channels pass through unmodified."
    explanation: "frame1_block fixes the channel count to 1 (mono). In a stereo network, only channel 0 is processed by the children. Channel 1 passes through without modification, which is rarely the intended behaviour."
llmRef: |
  container.frame1_block

  Serial container that converts block-based processing to per-sample (frame) processing with a fixed channel count of 1 (mono). Children receive one sample at a time and process it serially.

  Signal flow:
    audio in (channel 0 only) -> per-sample iteration -> children process each sample serially -> audio out (channel 0 processed, others unmodified)

  CPU: medium, monophonic. Significant per-sample call overhead compared to block processing.

  Parameters: none

  When to use:
    When per-sample accuracy is required for mono processing, such as a mono waveshaper or custom filter. In stereo networks, use frame2_block instead.

  Common mistakes:
    Only processes channel 0 in a stereo network. Use frame2_block for stereo.

  See also:
    [disambiguation] container.frame2_block - stereo per-sample processing
    [alternative] container.framex_block - dynamic channel count
    [disambiguation] container.fix8_block - block chunking as lower-cost alternative
---

The `frame1_block` container converts block-based processing to per-sample processing with a fixed channel count of 1 (mono). Children receive a single sample at a time and process it serially, with a block size of 1 in their preparation context. This enables true sample-accurate processing for algorithms that require it, such as custom filters, waveshapers, or feedback loops.

This node only processes channel 0. In a stereo network, channel 1 passes through unmodified. For stereo per-sample processing, use [frame2_block]($SN.container.frame2_block$) instead. Per-sample processing carries significant CPU overhead compared to block processing - each sample requires a full iteration through all child nodes. For most use cases, a [fix8_block]($SN.container.fix8_block$) container provides sufficient modulation precision at a fraction of the cost.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.processFrame:
      desc: "Processes all child nodes serially on a single mono sample"
---

```
// container.frame1_block - mono per-sample processing
// audio in -> audio out (channel 0 only)

process(input) {
    for each sample in channel 0:
        frame = [sample]
        children.processFrame(frame)
    // other channels pass through unmodified
}
```

::

### Performance

The CPU overhead of per-sample processing is substantial, especially in interpreted mode. Each sample in the buffer triggers a full call chain through all child nodes. In compiled mode, the compiler can inline much of this overhead. For a 512-sample buffer, frame1_block calls `processFrame()` 512 times compared to [fix8_block]($SN.container.fix8_block$)'s 64 calls to `process()`.

When bypassed, children revert to block processing with the original block size and process the full host buffer, allowing A/B comparison of per-sample vs block processing.

**See also:** $SN.container.frame2_block$ -- stereo per-sample processing, $SN.container.framex_block$ -- dynamic channel count, $SN.container.fix8_block$ -- block chunking as lower-cost alternative
