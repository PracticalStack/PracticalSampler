---
title: frame2_block
description: "Enables per-sample processing for child nodes on two stereo channels."
factoryPath: container.frame2_block
factory: container
polyphonic: false
tags: [container, frame, per-sample, stereo]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.frame1_block", type: disambiguation, reason: "Mono per-sample processing (single channel only)" }
  - { id: "container.framex_block", type: alternative, reason: "Dynamic channel count per-sample processing" }
  - { id: "container.fix8_block", type: disambiguation, reason: "Block chunking as a lower-cost alternative to per-sample processing" }
llmRef: |
  container.frame2_block

  Serial container that converts block-based processing to per-sample (frame) processing with a fixed channel count of 2 (stereo). Children receive an interleaved stereo pair [left, right] for each sample.

  Signal flow:
    audio in (stereo) -> per-sample iteration -> children process each [left, right] pair serially -> audio out

  CPU: medium, monophonic. Significant per-sample call overhead. Slightly more than frame1_block due to stereo interleaving.

  Parameters: none

  When to use:
    Standard frame container for stereo networks. Use when per-sample accuracy is needed for stereo DSP such as filters, waveshapers, or feedback algorithms. The most commonly used frame container.

  Key details:
    SNEX templated processFrame generates overloads for all channel counts. Use explicit per-channel overloads to avoid mono compile errors.

  See also:
    [disambiguation] container.frame1_block - mono per-sample processing
    [alternative] container.framex_block - dynamic channel count
    [disambiguation] container.fix8_block - block chunking as lower-cost alternative
---

The `frame2_block` container converts block-based processing to per-sample processing with a fixed channel count of 2 (stereo). Children receive an interleaved pair of [left, right] samples for each time step, enabling true sample-accurate stereo processing. This is the most commonly used frame container, appearing in the majority of networks that require per-sample DSP.

Per-sample processing carries significant CPU overhead compared to block processing. Each sample requires interleaving both channels into a frame, processing through all children, then deinterleaving back. For most modulation-precision needs, a [fix8_block]($SN.container.fix8_block$) container provides sufficient accuracy at a fraction of the cost. Reserve `frame2_block` for algorithms that genuinely require sample-by-sample computation, such as custom filters, waveshapers with feedback, or phase-sensitive effects.

## Signal Path

::signal-path
---
glossary:
  functions:
    children.processFrame:
      desc: "Processes all child nodes serially on a single stereo sample pair"
---

```
// container.frame2_block - stereo per-sample processing
// audio in -> audio out

process(input) {
    for each sample:
        frame = [left, right]
        children.processFrame(frame)
}
```

::

Unlike [frame1_block]($SN.container.frame1_block$) which only processes channel 0, this node processes both stereo channels. Channels beyond the first two are zeroed, not passed through. If the network has more than two channels, use [framex_block]($SN.container.framex_block$) to process all of them. In compiled mode, the fixed stereo channel count allows the compiler to fully optimise the interleaving loop, making `frame2_block` slightly more efficient than [framex_block]($SN.container.framex_block$) for stereo networks.

### SNEX Considerations

When writing SNEX nodes with a templated `processFrame` function, the compiler generates overloads for every channel count (mono and stereo). If the template body accesses `data[1]`, the mono overload triggers a compile error. To fix this, replace the template with explicit overloads: a no-op `processFrame(span<float, 1>&)` and the real logic in `processFrame(span<float, 2>&)`.

When bypassed, children revert to block processing with the original block size and process the full host buffer, allowing A/B comparison of per-sample vs block processing.

**See also:** $SN.container.frame1_block$ -- mono per-sample processing, $SN.container.framex_block$ -- dynamic channel count, $SN.container.fix8_block$ -- block chunking as lower-cost alternative
