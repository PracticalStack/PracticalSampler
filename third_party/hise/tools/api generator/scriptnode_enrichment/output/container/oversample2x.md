---
title: Oversample 2x
description: "Upsamples the audio signal by a fixed factor of 2, processes child nodes at the higher rate, then downsamples back."
factoryPath: container.oversample2x
factory: container
polyphonic: false
tags: [container, oversampling, aliasing]
screenshot: /images/v2/reference/scriptnodes/container/oversample2x.png
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors:
    - { parameter: FilterType, impact: minor, note: "FIR is slightly more expensive than Polyphase but negligible compared to child processing cost." }
seeAlso:
  - { id: "container.oversample", type: disambiguation, reason: "Dynamic variant with selectable factor" }
  - { id: "container.oversample4x", type: disambiguation, reason: "Fixed 4x variant for stronger aliasing reduction" }
  - { id: "container.repitch", type: alternative, reason: "Resampling alternative - changes pitch rather than preventing aliasing" }
commonMistakes:
  - title: "Nesting oversample containers is not allowed"
    wrong: "Placing an oversample2x container inside another oversample container"
    right: "Use a single oversample container with the combined factor you need."
    explanation: "The container checks that the incoming sample rate matches the network's original sample rate. If the signal has already been upsampled, this check fails and the node reports an error."
  - title: "Cannot be used in polyphonic networks"
    wrong: "Placing an oversample2x container inside a polyphonic signal chain"
    right: "Use oversampling only in monophonic contexts."
    explanation: "Oversampling is restricted to monophonic processing. If the container detects a polyphonic voice context, it reports an error."
llmRef: |
  container.oversample2x

  Upsamples the audio signal by a fixed factor of 2, processes child nodes serially at the higher sample rate, then downsamples back with anti-aliasing filtering. Fixed variant of container.oversample - no factor parameter, slightly lower overhead.

  Signal flow:
    audio in -> upsample 2x -> child1 -> child2 -> ... -> childN -> downsample 2x -> audio out

  CPU: medium, monophonic only. Child processing cost is always doubled.

  Parameters:
    FilterType (Polyphase / FIR, default Polyphase) - anti-aliasing filter. Polyphase has lower latency, FIR has steeper rolloff.

  Constraints:
    - Monophonic only (no polyphonic voice context)
    - Block processing only (no frame processing)
    - Cannot be nested inside another oversample container
    - Latency from the anti-aliasing filter is not reported to the host

  When to use:
    Mild aliasing reduction for waveshaping or saturation where 2x is sufficient. At 44.1 kHz, children process at 88.2 kHz. Use the dynamic container.oversample variant if the factor needs to be configurable.

  Common mistakes:
    Cannot nest oversample containers - use a single higher factor instead.
    Cannot be used in polyphonic networks.

  See also:
    disambiguation container.oversample - dynamic variant with selectable factor
    disambiguation container.oversample4x - fixed 4x variant
    alternative container.repitch - resampling alternative
---

Upsamples the audio signal by a fixed factor of 2, processes all child nodes serially at the higher sample rate, then downsamples the result back to the original rate with anti-aliasing filtering. This is the lightest oversampling option, doubling both the sample rate and the block size for all children.

This is a fixed-factor variant of the [oversample]($SN.container.oversample$) container. It does not expose an Oversampling parameter - the factor is always 2x. This avoids the overhead of runtime factor switching and the brief audio interruption that the dynamic variant incurs when changing factors. At a base sample rate of 44.1 kHz, child nodes process at 88.2 kHz. Use this when mild aliasing reduction is sufficient, for example with gentle saturation or waveshaping that does not generate strong high-order harmonics.

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
      desc: "Doubles the sample rate and block size"
    downsample:
      desc: "Halves the sample rate and block size back to the original values with anti-aliasing filtering"
---

```
// container.oversample2x - fixed 2x oversampling container
// audio in -> audio out (children process at 2x rate)

dispatch(input) {
    upsampled = upsample(input, 2, FilterType)
    // children process serially at sampleRate * 2, blockSize * 2
    for each child in chain:
        child.process(upsampled)
    output = downsample(upsampled, 2, FilterType)
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

**See also:** $SN.container.oversample$ -- dynamic variant with selectable factor, $SN.container.oversample4x$ -- fixed 4x variant for stronger aliasing reduction, $SN.container.repitch$ -- resampling alternative that changes pitch