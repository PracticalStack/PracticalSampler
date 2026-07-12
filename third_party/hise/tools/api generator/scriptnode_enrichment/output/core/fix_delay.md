---
title: Fix Delay
description: "A non-interpolating delay line with crossfade smoothing for delay time changes."
factoryPath: core.fix_delay
factory: core
polyphonic: false
tags: [core, delay, fixed, crossfade]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "jdsp.jdelay", type: alternative, reason: "Interpolating delay line suitable for modulation effects like chorus and phaser" }
  - { id: "Delay", type: module, reason: "Module-tree delay with feedback and tempo sync" }
commonMistakes:
  - title: "Using fix_delay for modulation effects"
    wrong: "Modulating DelayTime rapidly to create chorus or phaser effects"
    right: "Use an interpolating delay line such as jdsp.jdelay for pitch-modulated effects."
    explanation: "This delay quantises to integer samples with no fractional interpolation. Rapid modulation produces audible stepping artefacts rather than smooth pitch changes."
  - title: "FadeTime is in samples, not milliseconds"
    wrong: "Setting FadeTime to 100 expecting 100 ms of crossfade"
    right: "FadeTime is in samples. At 44100 Hz, 512 samples is roughly 11.6 ms."
    explanation: "The FadeTime parameter uses samples as its unit. The duration in milliseconds depends on the current sample rate."
llmRef: |
  core.fix_delay

  A non-interpolating delay line. Delays the input signal by a configurable duration with crossfade smoothing when the delay time changes.

  Signal flow:
    audio in -> delay line (integer sample offset) -> audio out

  CPU: low, monophonic

  Parameters:
    DelayTime (0 - 1000 ms, default 100): delay duration, quantised to integer samples
    FadeTime (0 - 1024 samples, default 512): crossfade length when delay time changes

  When to use:
    Frequently used (rank 23, 18 instances). Use for static delays, echo/feedback effects, or any scenario where sample-accurate delay is needed without pitch shifting. For modulation effects (chorus, phaser), use an interpolating delay instead.

  Common mistakes:
    Not suitable for modulation effects -- quantises to integer samples.
    FadeTime is in samples, not milliseconds.

  See also:
    [alternative] jdsp.jdelay -- interpolating delay for modulation effects
    [module] Delay -- module-tree delay with feedback and tempo sync
---

This node delays the input signal by a configurable amount, using a non-interpolating circular buffer. The delay time is quantised to the nearest integer sample, so fractional millisecond values are truncated. Each channel is delayed independently through its own delay line, but all channels share the same delay time setting.

When the delay time changes, the node crossfades between the old and new read positions over a configurable number of samples to prevent clicks. This makes it safe to automate or modulate the delay time at control rates. For feedback-based echo effects, combine this node with [routing.send]($SN.routing.send$) and [routing.receive]($SN.routing.receive$) to create a feedback signal path.

## Signal Path

::signal-path
---
glossary:
  parameters:
    DelayTime:
      desc: "Delay duration in milliseconds, quantised to integer samples"
      range: "0 - 1000 ms"
      default: "100"
    FadeTime:
      desc: "Crossfade length in samples when delay time changes"
      range: "0 - 1024 samples"
      default: "512"
  functions:
    delayLine:
      desc: "Non-interpolating circular buffer that reads from a fixed integer sample offset"
---

```
// core.fix_delay - non-interpolating delay
// audio in -> audio out

process(input) {
    // per channel:
    output = delayLine(input, DelayTime)
    // crossfades over FadeTime samples when DelayTime changes
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Delay
    params:
      - { name: DelayTime, desc: "Delay duration. Quantised to integer samples internally. The skewed range centres around 100 ms for fine control at short delay times.", range: "0 - 1000 ms", default: "100" }
      - { name: FadeTime, desc: "Crossfade length when delay time changes, to avoid clicks. Specified in samples.", range: "0 - 1024 samples", default: "512" }
---
::

### Limitations

The maximum delay time is 1000 ms. For longer delays, consider using multiple delay nodes in series or an alternative approach.

Changing the channel configuration (e.g. moving the node between containers with different channel counts) clears the delay buffer.

**See also:** $SN.jdsp.jdelay$ -- interpolating delay suitable for chorus and phaser effects, $MODULES.Delay$ -- module-tree delay with feedback and tempo sync
