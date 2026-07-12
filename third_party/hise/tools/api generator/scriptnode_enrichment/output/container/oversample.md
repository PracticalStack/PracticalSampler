---
title: Oversample
description: "Upsamples the audio signal by a selectable factor, processes child nodes at the higher rate, then downsamples back."
factoryPath: container.oversample
factory: container
polyphonic: false
tags: [container, oversampling, aliasing]
screenshot: /images/v2/reference/scriptnodes/container/oversample.png
cpuProfile:
  baseline: high
  polyphonic: false
  scalingFactors:
    - { parameter: Oversampling, impact: multiplicative, note: "CPU scales linearly with the oversampling factor. 16x = 16x the processing cost for all child nodes." }
    - { parameter: FilterType, impact: minor, note: "FIR is slightly more expensive than Polyphase but negligible compared to child processing cost." }
seeAlso:
  - { id: "container.oversample2x", type: disambiguation, reason: "Fixed 2x variant - no factor parameter, slightly lower overhead" }
  - { id: "container.oversample4x", type: disambiguation, reason: "Fixed 4x variant" }
  - { id: "container.oversample8x", type: disambiguation, reason: "Fixed 8x variant" }
  - { id: "container.oversample16x", type: disambiguation, reason: "Fixed 16x variant" }
  - { id: "container.repitch", type: alternative, reason: "Resampling alternative - changes pitch rather than preventing aliasing" }
commonMistakes:
  - title: "Changing the factor during playback causes a gap"
    wrong: "Modulating the Oversampling parameter at audio rate or during playback"
    right: "Set the oversampling factor before processing begins, or accept a brief audio interruption when changing it."
    explanation: "Changing the oversampling factor triggers a full re-preparation of the entire child chain. Audio output is briefly interrupted during this process. Treat the Oversampling parameter as a configuration setting, not a real-time control."
  - title: "Nesting oversample containers is not allowed"
    wrong: "Placing an oversample container inside another oversample container"
    right: "Use a single oversample container with the combined factor you need (e.g. 8x instead of nested 2x and 4x)."
    explanation: "The container checks that the incoming sample rate matches the network's original sample rate. If the signal has already been upsampled, this check fails and the node reports an error."
  - title: "Cannot be used in polyphonic networks"
    wrong: "Placing an oversample container inside a polyphonic signal chain"
    right: "Use oversampling only in monophonic contexts. For polyphonic aliasing reduction, apply oversampling to the entire network output instead."
    explanation: "Oversampling is restricted to monophonic processing. If the container detects a polyphonic voice context, it reports an error."
llmRef: |
  container.oversample

  Upsamples the audio signal by a selectable factor (1x to 16x), processes child nodes serially at the higher sample rate, then downsamples back with anti-aliasing filtering. Used to reduce aliasing in nonlinear processing such as waveshaping, saturation, and distortion.

  Signal flow:
    audio in -> upsample by factor -> child1 -> child2 -> ... -> childN -> downsample by factor -> audio out

  CPU: high, monophonic only. Scales linearly with oversampling factor.

  Parameters:
    Oversampling (0 - 4, step 1, default 1 / 2x) - factor exponent. 0=None, 1=2x, 2=4x, 3=8x, 4=16x. Changes cause brief audio interruption.
    FilterType (Polyphase / FIR, default Polyphase) - anti-aliasing filter. Polyphase has lower latency, FIR has steeper rolloff.

  Constraints:
    - Monophonic only (no polyphonic voice context)
    - Block processing only (no frame processing)
    - Cannot be nested inside another oversample container
    - Latency from the anti-aliasing filter is not reported to the host

  When to use:
    When child nodes produce harmonics that fold back as aliasing (waveshaping, saturation, FM synthesis). Choose this dynamic variant when the factor needs to be configurable. Use the fixed variants (oversample2x, oversample4x, etc.) when the factor is known at design time.

  Key details:
    Bypassing only removes upsampling/downsampling - children still process at original rate (no CPU saving).
    For gap-free runtime factor switching, use ScriptNode.setParent() to reparent the signal chain.

  Common mistakes:
    Changing the factor during playback causes a brief audio gap.
    Cannot nest oversample containers - use a single higher factor instead.
    Cannot be used in polyphonic networks.
    Bypassing does not reduce CPU - children still run.

  See also:
    disambiguation container.oversample2x - fixed 2x variant
    disambiguation container.oversample4x - fixed 4x variant
    disambiguation container.oversample8x - fixed 8x variant
    disambiguation container.oversample16x - fixed 16x variant
    alternative container.repitch - resampling alternative
---

Upsamples the audio signal by a selectable factor, processes all child nodes serially at the higher sample rate, then downsamples the result back to the original rate with anti-aliasing filtering. This reduces aliasing artefacts from nonlinear processing such as waveshaping, saturation, and distortion, where harmonics generated above the Nyquist frequency would otherwise fold back into the audible range.

This is the dynamic variant, which exposes an Oversampling parameter to select the factor at runtime (None, 2x, 4x, 8x, or 16x). Both the sample rate and the block size are multiplied by the selected factor for all child nodes. Two anti-aliasing filter types are available: Polyphase (lower latency) and FIR (steeper rolloff near Nyquist). For a fixed oversampling factor with slightly lower overhead, use the dedicated [oversample2x]($SN.container.oversample2x$), [oversample4x]($SN.container.oversample4x$), [oversample8x]($SN.container.oversample8x$), or [oversample16x]($SN.container.oversample16x$) variants instead.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Oversampling:
      desc: "Oversampling factor exponent (0=None, 1=2x, 2=4x, 3=8x, 4=16x)"
      range: "0 - 4"
      default: "1 (2x)"
    FilterType:
      desc: "Anti-aliasing filter type: Polyphase (lower latency) or FIR (steeper rolloff)"
      range: "Polyphase / FIR"
      default: "Polyphase"
  functions:
    upsample:
      desc: "Increases the sample rate and block size by the oversampling factor"
    downsample:
      desc: "Reduces the sample rate and block size back to the original values with anti-aliasing filtering"
---

```
// container.oversample - dynamic oversampling container
// audio in -> audio out (children process at higher rate)

dispatch(input) {
    factor = pow(2, Oversampling)    // 1, 2, 4, 8, or 16

    upsampled = upsample(input, factor, FilterType)
    // children process serially at sampleRate * factor, blockSize * factor
    for each child in chain:
        child.process(upsampled)
    output = downsample(upsampled, factor, FilterType)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: Oversampling, desc: "The oversampling factor as an exponent of 2. None (1x) passes audio through with minimal overhead. Higher factors improve aliasing reduction but multiply CPU cost linearly.", range: "None / 2x / 4x / 8x / 16x", default: "2x" }
      - { name: FilterType, desc: "The anti-aliasing filter used during up- and downsampling. Polyphase has lower latency and is suitable for most cases. FIR has a steeper rolloff near Nyquist, which can be useful when the oversampled processing generates content very close to the folding frequency.", range: "Polyphase / FIR", default: "Polyphase" }
---
::

When the Oversampling parameter is set to None (1x), the up- and downsampling stages still execute as a trivial passthrough. This adds negligible overhead compared to not using the container at all, but is not zero-cost.

### Bypass Behaviour

When bypassed, child nodes revert to the original sample rate and block size but continue to process audio normally. Bypassing only removes the upsampling/downsampling step -- it does not reduce CPU usage. The container re-prepares the entire child chain on bypass state changes, so toggling bypass is not instantaneous.

### Latency

The anti-aliasing filter introduces a small amount of latency that is not reported to the host or the surrounding network. For most use cases this is inaudible, but it may be relevant in parallel signal paths where phase alignment matters.

**See also:** $SN.container.oversample2x$ -- fixed 2x variant, $SN.container.oversample4x$ -- fixed 4x variant, $SN.container.oversample8x$ -- fixed 8x variant, $SN.container.oversample16x$ -- fixed 16x variant, $SN.container.repitch$ -- resampling alternative that changes pitch