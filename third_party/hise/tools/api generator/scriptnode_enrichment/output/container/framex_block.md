---
title: framex_block
description: "Enables per-sample processing for child nodes with a dynamic channel count that adapts to the network context."
factoryPath: container.framex_block
factory: container
polyphonic: false
tags: [container, frame, per-sample, dynamic-channels]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.frame2_block", type: alternative, reason: "Fixed stereo per-sample processing with better compilation optimisation" }
  - { id: "container.dynamic_blocksize", type: companion, reason: "dynamic_blocksize with BlockSize index 0 is functionally equivalent" }
llmRef: |
  container.framex_block

  Serial container that converts block-based processing to per-sample (frame) processing with a dynamic channel count. Adapts to the current network channel count at runtime (up to 8 channels).

  Signal flow:
    audio in (all channels) -> per-sample iteration -> children process each multi-channel frame serially -> audio out

  CPU: medium, monophonic. Per-sample call overhead plus marginal runtime channel dispatch cost.

  Parameters: none

  When to use:
    When per-sample processing is needed and the channel count is not known at design time, or when processing more than 2 channels. For standard stereo networks, prefer frame2_block for better compiled performance.

  See also:
    [alternative] container.frame2_block - fixed stereo, better optimisation
    [companion] container.dynamic_blocksize - BlockSize index 0 is equivalent
---

The `framex_block` container converts block-based processing to per-sample processing with a channel count that adapts to the network context at runtime. Unlike [frame1_block]($SN.container.frame1_block$) and [frame2_block]($SN.container.frame2_block$) which fix the channel count to 1 or 2, this node processes all available channels (up to 8 by default). Children receive a multi-channel frame for each sample containing one value per channel.

For standard stereo networks, prefer [frame2_block]($SN.container.frame2_block$). The fixed channel count allows the compiler to fully optimise the interleaving loop in compiled output. `framex_block` uses a runtime channel dispatch that prevents some of these optimisations. The difference is small for 1-2 channels but grows with higher channel counts.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.processFrame:
      desc: "Processes all child nodes serially on a single multi-channel frame"
---

```
// container.framex_block - dynamic-channel per-sample processing
// audio in -> audio out (all channels)

process(input) {
    for each sample:
        frame = [ch0, ch1, ..., chN]    // all channels
        children.processFrame(frame)
}
```

::

### Limitations

The maximum supported channel count is 8 by default. Processing more channels triggers a debug assertion. This limit covers all standard use cases including surround configurations.

This node is functionally equivalent to [dynamic_blocksize]($SN.container.dynamic_blocksize$) with its BlockSize parameter set to index 0 (block size 1) -- both use the same frame-based processing path with runtime channel dispatch. When bypassed, children revert to block processing with the original block size and process the full host buffer.

**See also:** $SN.container.frame2_block$ -- fixed stereo per-sample processing with better optimisation, $SN.container.dynamic_blocksize$ -- BlockSize index 0 is functionally equivalent
