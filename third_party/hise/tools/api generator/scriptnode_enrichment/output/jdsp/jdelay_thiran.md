---
title: Delay (Thiran)
description: "A delay line with Thiran allpass interpolation -- flat frequency response and low CPU cost, but not suitable for fast delay time modulation."
factoryPath: jdsp.jdelay_thiran
factory: jdsp
polyphonic: true
tags: [jdsp, delay, time-based]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: Limit, impact: memory, note: "Larger values allocate more buffer memory" }
seeAlso:
  - { id: "jdsp.jdelay", type: alternative, reason: "Cheaper with mild HF roll-off, better for modulation" }
  - { id: "jdsp.jdelay_cubic", type: alternative, reason: "Best modulation behaviour but higher CPU" }
  - { id: "core.fix_delay", type: alternative, reason: "Built-in parameter smoothing for knob-controlled delay" }
commonMistakes:
  - title: "Not suitable for modulated delay effects"
    wrong: "Using jdelay_thiran for chorus or flanger effects with rapidly changing delay time"
    right: "Use jdelay_cubic for modulated effects. jdelay_thiran is best for static or slowly changing delay times."
    explanation: "The Thiran interpolator uses an internal allpass filter whose state needs time to settle after coefficient changes. Rapid delay time modulation causes audible artefacts."
  - title: "No built-in delay time smoothing"
    wrong: "Connecting a knob directly to the DelayTime parameter"
    right: "Place a smoothed_parameter node between the control source and DelayTime, or use core.fix_delay which has built-in smoothing."
    explanation: "The delay time is applied immediately without ramping. Abrupt changes produce audible clicks."
llmRef: |
  jdsp.jdelay_thiran

  A delay line using Thiran allpass interpolation. Flat amplitude response with low CPU cost, but NOT suitable for fast delay time modulation -- the allpass filter produces artefacts when its coefficients change rapidly. Polyphonic.

  Signal flow:
    audio in -> per-channel delay with Thiran allpass interpolation -> audio out

  CPU: low, polyphonic

  Parameters:
    Limit (0 - 1000 ms mono / 0 - 30 ms poly, default max) - maximum delay buffer size
    DelayTime (0 - 1000 ms mono / 0 - 30 ms poly, default 0) - actual delay time; no smoothing

  When to use:
    Fixed or slowly changing delays where flat frequency response matters but fast modulation is not needed (e.g., comb filters, static delays in reverb networks). For modulated effects, use jdelay_cubic instead.

  See also:
    alternative jdsp.jdelay -- cheaper, mild HF roll-off, better for modulation
    alternative jdsp.jdelay_cubic -- best modulation, higher CPU
    alternative core.fix_delay -- delay with built-in smoothing
---

A delay line that uses Thiran allpass interpolation to calculate values between samples. Like the cubic variant, this produces a flat amplitude response with no high-frequency roll-off. The CPU cost is comparable to linear interpolation, making it an efficient choice for applications that need tonal accuracy.

However, the Thiran interpolator uses an internal allpass filter that has state which needs time to settle after coefficient changes. Rapidly modulating the delay time causes the filter to produce transient artefacts, making this variant unsuitable for chorus, flanger, or other modulated delay effects. It is best suited for static or slowly changing delay times such as comb filters or fixed delays in reverb feedback networks.

Each voice maintains its own delay buffer. When used polyphonically, the maximum delay time is reduced from 1000 ms to 30 ms.

> [!Warning:Not for fast delay modulation] The Thiran allpass filter produces artefacts when the delay time changes rapidly. For chorus or flanger effects, use [jdelay_cubic]($SN.jdsp.jdelay_cubic$) instead.

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
    thiranDelay:
      desc: "Reads from the delay buffer using Thiran allpass interpolation"
---

```
// jdsp.jdelay_thiran - Thiran allpass interpolation delay
// audio in -> audio out (per voice)

process(input) {
    output = thiranDelay(input, DelayTime)
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
| [jdelay_cubic]($SN.jdsp.jdelay_cubic$) | Lagrange 3rd-order | Flat | Best | Medium |
| **jdelay_thiran** | Thiran allpass | Flat | Not suitable | Low |

**See also:** [$SN.jdsp.jdelay$]($SN.jdsp.jdelay$) -- cheaper with mild HF roll-off, better for modulation, [$SN.jdsp.jdelay_cubic$]($SN.jdsp.jdelay_cubic$) -- best modulation behaviour but higher CPU, [$SN.core.fix_delay$]($SN.core.fix_delay$) -- delay with built-in parameter smoothing
