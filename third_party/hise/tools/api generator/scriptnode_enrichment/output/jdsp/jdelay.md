---
title: Delay (Linear)
description: "A delay line with linear interpolation -- lowest CPU cost but mild high-frequency roll-off at non-integer delay times."
factoryPath: jdsp.jdelay
factory: jdsp
polyphonic: true
tags: [jdsp, delay, time-based]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: Limit, impact: memory, note: "Larger values allocate more buffer memory" }
seeAlso:
  - { id: "jdsp.jdelay_cubic", type: alternative, reason: "Flat frequency response but higher CPU cost" }
  - { id: "jdsp.jdelay_thiran", type: alternative, reason: "Flat frequency response and low CPU but not suitable for fast modulation" }
  - { id: "core.fix_delay", type: alternative, reason: "Built-in parameter smoothing for knob-controlled delay" }
  - { id: "Delay", type: module, reason: "JUCE-based delay with interpolation options" }
commonMistakes:
  - title: "No built-in delay time smoothing"
    wrong: "Connecting a knob directly to the DelayTime parameter"
    right: "Place a smoothed_parameter node between the control source and DelayTime, or use core.fix_delay which has built-in smoothing."
    explanation: "The delay time is applied immediately without ramping. Abrupt changes produce audible clicks. External smoothing is required for any continuous delay time control."
  - title: "Modulation needs frame processing"
    wrong: "Modulating DelayTime at audio rate without a frame container"
    right: "Place the delay and its modulation source inside a frame2_block container for sample-accurate delay time modulation."
    explanation: "Without frame processing, delay time updates occur once per block. For smooth audio-rate modulation (chorus, flanger), frame-by-frame processing is essential."
llmRef: |
  jdsp.jdelay

  A delay line using linear interpolation between samples. Lowest CPU cost of the three jdelay variants, but non-integer delay times cause mild high-frequency roll-off (inherent to linear interpolation). Polyphonic -- each voice has an independent delay buffer.

  Signal flow:
    audio in -> per-channel delay with linear interpolation -> audio out

  CPU: low, polyphonic

  Parameters:
    Limit (0 - 1000 ms mono / 0 - 30 ms poly, default max) - maximum delay buffer size; affects memory, not CPU
    DelayTime (0 - 1000 ms mono / 0 - 30 ms poly, default 0) - actual delay time; no smoothing applied

  Polyphonic range:
    When used polyphonically, the parameter range is reduced from 0-1000 ms to 0-30 ms to limit per-voice memory usage.

  When to use:
    General-purpose delay where mild high-frequency colouring is acceptable. For modulated delays (chorus/flanger) where flat response matters, use jdelay_cubic. For fixed delays where modulation stability is not needed, jdelay_thiran offers flat response at lower CPU cost.

  Common mistakes:
    No smoothing on DelayTime -- use smoothed_parameter or core.fix_delay.
    Audio-rate modulation needs frame2_block container.

  See also:
    alternative jdsp.jdelay_cubic -- flat response, higher CPU
    alternative jdsp.jdelay_thiran -- flat response, not for fast modulation
    alternative core.fix_delay -- delay with built-in smoothing
    [module] Delay -- module-tree stereo delay with tempo sync
---

A delay line that uses linear interpolation to calculate values between samples. This is the cheapest of the three delay line variants, but non-integer delay times cause a mild high-frequency roll-off because linear interpolation acts as a low-pass filter. The effect is more noticeable at lower sample rates and with rapidly modulated delay times.

Each voice maintains its own delay buffer, so the node can be used in polyphonic contexts where each voice needs an independent delay. When used polyphonically, the maximum delay time is reduced from 1000 ms to 30 ms to keep per-voice memory usage reasonable.

> [!Warning:No delay time smoothing] The delay time is applied immediately without any ramping. Connect a [smoothed_parameter]($SN.control.smoothed_parameter$) node before the DelayTime input to prevent clicks, or use [core.fix_delay]($SN.core.fix_delay$) which includes built-in smoothing.

> [!Tip:Use frame processing for modulation effects] For chorus or flanger effects, place the delay node and its modulation source inside a [frame2_block]($SN.container.frame2_block$) container. This ensures delay time updates happen every sample rather than once per block.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Limit:
      desc: "Maximum delay buffer size (affects memory allocation)"
      range: "0 - 1000 ms (mono) / 0 - 30 ms (poly)"
      default: "max"
    DelayTime:
      desc: "Actual delay time applied to the signal"
      range: "0 - 1000 ms (mono) / 0 - 30 ms (poly)"
      default: "0"
  functions:
    linearDelay:
      desc: "Reads from the delay buffer using linear interpolation between adjacent samples"
---

```
// jdsp.jdelay - linear interpolation delay
// audio in -> audio out (per voice)

process(input) {
    output = linearDelay(input, DelayTime)
    // Limit sets buffer size; DelayTime must not exceed Limit
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Delay Configuration
    params:
      - { name: Limit, desc: "Maximum delay time in milliseconds. Sets the delay buffer size. Larger values use more memory but do not affect CPU cost. When used polyphonically, the range is reduced to 0-30 ms.", range: "0 - 1000 ms (mono) / 0 - 30 ms (poly)", default: "1000 (mono) / 30 (poly)" }
      - { name: DelayTime, desc: "The actual delay time in milliseconds. Applied immediately without smoothing. Must not exceed the Limit value.", range: "0 - 1000 ms (mono) / 0 - 30 ms (poly)", default: "0" }
---
::

### Interpolation Comparison

The three jdelay variants share identical parameters and differ only in how they calculate values between samples:

| Variant | Interpolation | Frequency Response | Modulation | CPU |
|---------|--------------|-------------------|------------|-----|
| **jdelay** | Linear | High-frequency roll-off | Acceptable | Low |
| [jdelay_cubic]($SN.jdsp.jdelay_cubic$) | Lagrange 3rd-order | Flat | Best | Medium |
| [jdelay_thiran]($SN.jdsp.jdelay_thiran$) | Thiran allpass | Flat | Not suitable | Low |

**See also:** [$SN.jdsp.jdelay_cubic$]($SN.jdsp.jdelay_cubic$) -- flat frequency response, highest CPU, [$SN.jdsp.jdelay_thiran$]($SN.jdsp.jdelay_thiran$) -- flat response, not for fast modulation, [$SN.core.fix_delay$]($SN.core.fix_delay$) -- delay with built-in parameter smoothing, $MODULES.Delay$ -- module-tree stereo delay with tempo sync
