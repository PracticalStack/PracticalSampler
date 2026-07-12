---
title: Delay (Cubic)
description: "A delay line with cubic Lagrange interpolation -- flat frequency response and best modulation behaviour, but highest CPU cost."
factoryPath: jdsp.jdelay_cubic
factory: jdsp
polyphonic: true
tags: [jdsp, delay, time-based]
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors:
    - { parameter: Limit, impact: memory, note: "Larger values allocate more buffer memory" }
seeAlso:
  - { id: "jdsp.jdelay", type: alternative, reason: "Lower CPU cost but mild high-frequency roll-off" }
  - { id: "jdsp.jdelay_thiran", type: alternative, reason: "Flat response and lower CPU but not suitable for fast modulation" }
  - { id: "core.fix_delay", type: alternative, reason: "Built-in parameter smoothing for knob-controlled delay" }
commonMistakes:
  - title: "No built-in delay time smoothing"
    wrong: "Connecting a knob directly to the DelayTime parameter"
    right: "Place a smoothed_parameter node between the control source and DelayTime, or use core.fix_delay which has built-in smoothing."
    explanation: "The delay time is applied immediately without ramping. Abrupt changes produce audible clicks. External smoothing is required for any continuous delay time control."
llmRef: |
  jdsp.jdelay_cubic

  A delay line using 3rd-order Lagrange (cubic) interpolation. Flat amplitude response across the audio range with no high-frequency colouring. Best suited for modulated delay effects (chorus, flanger) where tonal accuracy matters. Highest CPU cost of the three variants. Polyphonic.

  Signal flow:
    audio in -> per-channel delay with cubic interpolation -> audio out

  CPU: medium, polyphonic

  Parameters:
    Limit (0 - 1000 ms mono / 0 - 30 ms poly, default max) - maximum delay buffer size
    DelayTime (0 - 1000 ms mono / 0 - 30 ms poly, default 0) - actual delay time; no smoothing

  When to use:
    Modulated delay effects where flat frequency response is important (chorus, flanger, pitch shifting). For fixed delays where modulation quality does not matter, use jdelay (cheaper) or jdelay_thiran (flat + cheaper).

  See also:
    alternative jdsp.jdelay -- cheaper, mild HF roll-off
    alternative jdsp.jdelay_thiran -- flat, cheaper, not for modulation
    alternative core.fix_delay -- delay with built-in smoothing
---

A delay line that uses 3rd-order Lagrange (cubic) interpolation to calculate values between samples. This produces a flat amplitude response across the audio range -- unlike linear interpolation, there is no high-frequency roll-off at non-integer delay times.

The cubic interpolation uses a four-point polynomial fit, making it the most CPU-intensive of the three delay variants. However, it handles rapid delay time changes cleanly because it has no internal filter state that needs to settle, making it the best choice for modulated delay effects such as chorus and flanger.

Each voice maintains its own delay buffer. When used polyphonically, the maximum delay time is reduced from 1000 ms to 30 ms.

> [!Warning:No delay time smoothing] The delay time is applied immediately without any ramping. Connect a [smoothed_parameter]($SN.control.smoothed_parameter$) node before the DelayTime input to prevent clicks.

> [!Tip:Best variant for modulation effects] For chorus and flanger effects where tonal accuracy matters, jdelay_cubic is the preferred choice. Its flat frequency response and clean modulation behaviour outweigh the higher CPU cost.

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
    cubicDelay:
      desc: "Reads from the delay buffer using 3rd-order Lagrange interpolation (four-point polynomial fit)"
---

```
// jdsp.jdelay_cubic - cubic interpolation delay
// audio in -> audio out (per voice)

process(input) {
    output = cubicDelay(input, DelayTime)
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

| Variant | Interpolation | Frequency Response | Modulation | CPU |
|---------|--------------|-------------------|------------|-----|
| [jdelay]($SN.jdsp.jdelay$) | Linear | High-frequency roll-off | Acceptable | Low |
| **jdelay_cubic** | Lagrange 3rd-order | Flat | Best | Medium |
| [jdelay_thiran]($SN.jdsp.jdelay_thiran$) | Thiran allpass | Flat | Not suitable | Low |

**See also:** [$SN.jdsp.jdelay$]($SN.jdsp.jdelay$) -- lower CPU cost with mild high-frequency roll-off, [$SN.jdsp.jdelay_thiran$]($SN.jdsp.jdelay_thiran$) -- flat response and lower CPU but not for fast modulation, [$SN.core.fix_delay$]($SN.core.fix_delay$) -- delay with built-in parameter smoothing
