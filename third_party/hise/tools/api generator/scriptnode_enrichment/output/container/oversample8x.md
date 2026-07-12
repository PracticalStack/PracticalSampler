---
title: Oversample 8x
description: "Upsamples the audio signal by a fixed factor of 8, processes child nodes at the higher rate, then downsamples back."
factoryPath: container.oversample8x
factory: container
polyphonic: false
tags: [container, oversampling, aliasing]
screenshot: /images/v2/reference/scriptnodes/container/oversample8x.png
cpuProfile:
  baseline: very_high
  polyphonic: false
  scalingFactors:
    - { parameter: FilterType, impact: minor, note: "FIR is slightly more expensive than Polyphase but negligible compared to child processing cost." }
seeAlso:
  - { id: "container.oversample", type: disambiguation, reason: "Dynamic variant with selectable factor" }
  - { id: "container.oversample4x", type: disambiguation, reason: "Fixed 4x variant for lower CPU cost" }
  - { id: "container.oversample16x", type: disambiguation, reason: "Fixed 16x variant for maximum aliasing reduction" }
  - { id: "container.repitch", type: alternative, reason: "Resampling alternative - changes pitch rather than preventing aliasing" }
commonMistakes:
  - title: "Nesting oversample containers is not allowed"
    wrong: "Placing an oversample8x container inside another oversample container"
    right: "Use a single oversample container with the combined factor you need."
    explanation: "The container checks that the incoming sample rate matches the network's original sample rate. If the signal has already been upsampled, this check fails and the node reports an error."
  - title: "Cannot be used in polyphonic networks"
    wrong: "Placing an oversample8x container inside a polyphonic signal chain"
    right: "Use oversampling only in monophonic contexts."
    explanation: "Oversampling is restricted to monophonic processing. If the container detects a polyphonic voice context, it reports an error."
llmRef: |
  container.oversample8x

  Upsamples the audio signal by a fixed factor of 8, processes child nodes serially at the higher sample rate, then downsamples back with anti-aliasing filtering. Fixed variant of container.oversample - strong aliasing reduction at significant CPU cost.

  Signal flow:
    audio in -> upsample 8x -> child1 -> child2 -> ... -> childN -> downsample 8x -> audio out

  CPU: very_high, monophonic only. Child processing cost is multiplied by 8.

  Parameters:
    FilterType (Polyphase / FIR, default Polyphase) - anti-aliasing filter. Polyphase has lower latency, FIR has steeper rolloff.

  Constraints:
    - Monophonic only (no polyphonic voice context)
    - Block processing only (no frame processing)
    - Cannot be nested inside another oversample container
    - Latency from the anti-aliasing filter is not reported to the host

  When to use:
    Heavy nonlinear processing that generates strong high-order harmonics (aggressive waveshaping, hard clipping, FM synthesis). At 44.1 kHz, children process at 352.8 kHz. Consider whether 4x is sufficient before committing to this CPU cost.

  Common mistakes:
    Cannot nest oversample containers - use a single higher factor instead.
    Cannot be used in polyphonic networks.

  See also:
    disambiguation container.oversample - dynamic variant with selectable factor
    disambiguation container.oversample4x - fixed 4x variant
    disambiguation container.oversample16x - fixed 16x variant
    alternative container.repitch - resampling alternative
---

Upsamples the audio signal by a fixed factor of 8, processes all child nodes serially at the higher sample rate, then downsamples the result back to the original rate with anti-aliasing filtering. At a base sample rate of 44.1 kHz, child nodes process at 352.8 kHz. This provides strong aliasing reduction at significant CPU cost - all child processing is multiplied by 8.

This is a fixed-factor variant of the [oversample]($SN.container.oversample$) container. It does not expose an Oversampling parameter - the factor is always 8x. Both the sample rate and the block size are multiplied by 8 for all child nodes. Use this for heavy nonlinear processing that generates strong high-order harmonics, such as aggressive waveshaping, hard clipping, or FM synthesis. Consider whether [oversample4x]($SN.container.oversample4x$) provides sufficient quality before committing to the higher CPU cost.

## Signal Path

::signal-path
---
glossary:
  parameters:
    FilterType:
      desc: "Anti-aliasing filter type: Polyphase (lower latency) or FIR (steeper rolloff)"
      range: "Polyphase / FIR"
      default: "Polyphase"
  functions:
    upsample:
      desc: "Multiplies the sample rate and block size by 8"
    downsample:
      desc: "Reduces the sample rate and block size back to the original values with anti-aliasing filtering"
---

```
// container.oversample8x - fixed 8x oversampling container
// audio in -> audio out (children process at 8x rate)

dispatch(input) {
    upsampled = upsample(input, 8, FilterType)
    // children process serially at sampleRate * 8, blockSize * 8
    for each child in chain:
        child.process(upsampled)
    output = downsample(upsampled, 8, FilterType)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: FilterType, desc: "The anti-aliasing filter used during up- and downsampling. Polyphase has lower latency and is suitable for most cases. FIR has a steeper rolloff near Nyquist.", range: "Polyphase / FIR", default: "Polyphase" }
---
::

When bypassed, child nodes revert to the original sample rate and block size, and the container re-prepares the entire child chain on bypass state changes. The anti-aliasing filter introduces a small amount of latency that is not reported to the host or the surrounding network.

**See also:** $SN.container.oversample$ -- dynamic variant with selectable factor, $SN.container.oversample4x$ -- fixed 4x variant for lower CPU cost, $SN.container.oversample16x$ -- fixed 16x variant for maximum aliasing reduction, $SN.container.repitch$ -- resampling alternative that changes pitch