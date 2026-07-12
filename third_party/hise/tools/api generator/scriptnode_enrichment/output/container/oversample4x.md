---
title: Oversample 4x
description: "Upsamples the audio signal by a fixed factor of 4, processes child nodes at the higher rate, then downsamples back."
factoryPath: container.oversample4x
factory: container
polyphonic: false
tags: [container, oversampling, aliasing]
screenshot: /images/v2/reference/scriptnodes/container/oversample4x.png
cpuProfile:
  baseline: high
  polyphonic: false
  scalingFactors:
    - { parameter: FilterType, impact: minor, note: "FIR is slightly more expensive than Polyphase but negligible compared to child processing cost." }
seeAlso:
  - { id: "container.oversample", type: disambiguation, reason: "Dynamic variant with selectable factor" }
  - { id: "container.oversample2x", type: disambiguation, reason: "Fixed 2x variant for lighter processing" }
  - { id: "container.oversample8x", type: disambiguation, reason: "Fixed 8x variant for stronger aliasing reduction" }
  - { id: "container.repitch", type: alternative, reason: "Resampling alternative - changes pitch rather than preventing aliasing" }
commonMistakes:
  - title: "Nesting oversample containers is not allowed"
    wrong: "Placing an oversample4x container inside another oversample container"
    right: "Use a single oversample container with the combined factor you need."
    explanation: "The container checks that the incoming sample rate matches the network's original sample rate. If the signal has already been upsampled, this check fails and the node reports an error."
  - title: "Cannot be used in polyphonic networks"
    wrong: "Placing an oversample4x container inside a polyphonic signal chain"
    right: "Use oversampling only in monophonic contexts."
    explanation: "Oversampling is restricted to monophonic processing. If the container detects a polyphonic voice context, it reports an error."
llmRef: |
  container.oversample4x

  Upsamples the audio signal by a fixed factor of 4, processes child nodes serially at the higher sample rate, then downsamples back with anti-aliasing filtering. Fixed variant of container.oversample - good balance of aliasing reduction and CPU cost.

  Signal flow:
    audio in -> upsample 4x -> child1 -> child2 -> ... -> childN -> downsample 4x -> audio out

  CPU: high, monophonic only. Child processing cost is always quadrupled.

  Parameters:
    FilterType (Polyphase / FIR, default Polyphase) - anti-aliasing filter. Polyphase has lower latency, FIR has steeper rolloff.

  Constraints:
    - Monophonic only (no polyphonic voice context)
    - Block processing only (no frame processing)
    - Cannot be nested inside another oversample container
    - Latency from the anti-aliasing filter is not reported to the host

  When to use:
    Effective aliasing reduction for waveshaping, saturation, and distortion. At 44.1 kHz, children process at 176.4 kHz. Good balance between quality and CPU cost for most nonlinear processing.

  Common mistakes:
    Cannot nest oversample containers - use a single higher factor instead.
    Cannot be used in polyphonic networks.

  See also:
    disambiguation container.oversample - dynamic variant with selectable factor
    disambiguation container.oversample2x - fixed 2x variant
    disambiguation container.oversample8x - fixed 8x variant
    alternative container.repitch - resampling alternative
---

Upsamples the audio signal by a fixed factor of 4, processes all child nodes serially at the higher sample rate, then downsamples the result back to the original rate with anti-aliasing filtering. At a base sample rate of 44.1 kHz, child nodes process at 176.4 kHz. This provides a good balance between aliasing reduction and CPU cost for most nonlinear processing.

This is a fixed-factor variant of the [oversample]($SN.container.oversample$) container. It does not expose an Oversampling parameter - the factor is always 4x. Both the sample rate and the block size are quadrupled for all child nodes. Two anti-aliasing filter types are available: Polyphase (lower latency) and FIR (steeper rolloff near Nyquist). 4x oversampling is the most commonly used factor in practice, suitable for waveshaping, saturation, and distortion where 2x is not sufficient but 8x or 16x would be excessive.

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
      desc: "Quadruples the sample rate and block size"
    downsample:
      desc: "Reduces the sample rate and block size back to the original values with anti-aliasing filtering"
---

```
// container.oversample4x - fixed 4x oversampling container
// audio in -> audio out (children process at 4x rate)

dispatch(input) {
    upsampled = upsample(input, 4, FilterType)
    // children process serially at sampleRate * 4, blockSize * 4
    for each child in chain:
        child.process(upsampled)
    output = downsample(upsampled, 4, FilterType)
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

**See also:** $SN.container.oversample$ -- dynamic variant with selectable factor, $SN.container.oversample2x$ -- fixed 2x variant for lighter processing, $SN.container.oversample8x$ -- fixed 8x variant for stronger aliasing reduction, $SN.container.repitch$ -- resampling alternative that changes pitch