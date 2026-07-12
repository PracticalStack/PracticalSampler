---
title: Oversample 16x
description: "Upsamples the audio signal by the maximum fixed factor of 16, processes child nodes at the higher rate, then downsamples back."
factoryPath: container.oversample16x
factory: container
polyphonic: false
tags: [container, oversampling, aliasing]
screenshot: /images/v2/reference/scriptnodes/container/oversample16x.png
cpuProfile:
  baseline: very_high
  polyphonic: false
  scalingFactors:
    - { parameter: FilterType, impact: minor, note: "FIR is slightly more expensive than Polyphase but negligible compared to child processing cost." }
seeAlso:
  - { id: "container.oversample", type: disambiguation, reason: "Dynamic variant with selectable factor" }
  - { id: "container.oversample8x", type: disambiguation, reason: "Fixed 8x variant for lower CPU cost" }
  - { id: "container.oversample4x", type: disambiguation, reason: "Fixed 4x variant for moderate aliasing reduction" }
  - { id: "container.repitch", type: alternative, reason: "Resampling alternative - changes pitch rather than preventing aliasing" }
commonMistakes:
  - title: "Extreme CPU cost - verify 16x is necessary"
    wrong: "Using oversample16x by default for all nonlinear processing"
    right: "Start with oversample4x and only increase if audible aliasing remains. Reserve 16x for cases with extreme harmonic generation near Nyquist."
    explanation: "16x oversampling multiplies the CPU cost of all child nodes by 16. At 44.1 kHz, children process at 705.6 kHz. In most cases, 4x or 8x oversampling provides sufficient aliasing reduction at a fraction of the cost."
  - title: "Nesting oversample containers is not allowed"
    wrong: "Placing an oversample16x container inside another oversample container"
    right: "Use a single oversample container with the factor you need."
    explanation: "The container checks that the incoming sample rate matches the network's original sample rate. If the signal has already been upsampled, this check fails and the node reports an error."
  - title: "Cannot be used in polyphonic networks"
    wrong: "Placing an oversample16x container inside a polyphonic signal chain"
    right: "Use oversampling only in monophonic contexts."
    explanation: "Oversampling is restricted to monophonic processing. If the container detects a polyphonic voice context, it reports an error."
llmRef: |
  container.oversample16x

  Upsamples the audio signal by the maximum fixed factor of 16, processes child nodes serially at the higher sample rate, then downsamples back with anti-aliasing filtering. Fixed variant of container.oversample - maximum aliasing reduction at extreme CPU cost.

  Signal flow:
    audio in -> upsample 16x -> child1 -> child2 -> ... -> childN -> downsample 16x -> audio out

  CPU: very_high, monophonic only. Child processing cost is multiplied by 16.

  Parameters:
    FilterType (Polyphase / FIR, default Polyphase) - anti-aliasing filter. Polyphase has lower latency, FIR has steeper rolloff.

  Constraints:
    - Monophonic only (no polyphonic voice context)
    - Block processing only (no frame processing)
    - Cannot be nested inside another oversample container
    - Latency from the anti-aliasing filter is not reported to the host

  When to use:
    Only when lower factors leave audible aliasing. Typical cases: aggressive waveshaping with harmonics very close to Nyquist, extreme FM synthesis. At 44.1 kHz, children process at 705.6 kHz. Start with 4x and increase only if needed.

  Common mistakes:
    Extreme CPU cost - verify 16x is actually necessary before using.
    Cannot nest oversample containers.
    Cannot be used in polyphonic networks.

  See also:
    disambiguation container.oversample - dynamic variant with selectable factor
    disambiguation container.oversample8x - fixed 8x variant
    disambiguation container.oversample4x - fixed 4x variant
    alternative container.repitch - resampling alternative
---

Upsamples the audio signal by the maximum fixed factor of 16, processes all child nodes serially at the higher sample rate, then downsamples the result back to the original rate with anti-aliasing filtering. At a base sample rate of 44.1 kHz, child nodes process at 705.6 kHz. This is the highest available oversampling factor, providing the strongest aliasing reduction at extreme CPU cost - all child processing is multiplied by 16.

This is a fixed-factor variant of the [oversample]($SN.container.oversample$) container. It does not expose an Oversampling parameter - the factor is always 16x. Both the sample rate and the block size are multiplied by 16 for all child nodes. Reserve this for cases where lower factors leave audible aliasing, such as aggressive waveshaping with harmonics very close to Nyquist or extreme FM synthesis. In most cases, [oversample4x]($SN.container.oversample4x$) or [oversample8x]($SN.container.oversample8x$) provides sufficient quality at a fraction of the CPU cost.

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
      desc: "Multiplies the sample rate and block size by 16"
    downsample:
      desc: "Reduces the sample rate and block size back to the original values with anti-aliasing filtering"
---

```
// container.oversample16x - fixed 16x oversampling container
// audio in -> audio out (children process at 16x rate)

dispatch(input) {
    upsampled = upsample(input, 16, FilterType)
    // children process serially at sampleRate * 16, blockSize * 16
    for each child in chain:
        child.process(upsampled)
    output = downsample(upsampled, 16, FilterType)
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

**See also:** $SN.container.oversample$ -- dynamic variant with selectable factor, $SN.container.oversample8x$ -- fixed 8x variant for lower CPU cost, $SN.container.oversample4x$ -- fixed 4x variant for moderate aliasing reduction, $SN.container.repitch$ -- resampling alternative that changes pitch