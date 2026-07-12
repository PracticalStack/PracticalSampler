---
title: One Pole
description: "A first-order filter with lowpass and highpass modes, providing a gentle 6 dB/octave slope."
factoryPath: filters.one_pole
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/one_pole.png
polyphonic: true
tags: [filters, one-pole, lowpass, highpass, smoothing, dc-blocking]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.svf", type: alternative, reason: "Second-order SVF for steeper rolloff" }
commonMistakes: []
llmRef: |
  filters.one_pole

  First-order (one-pole) filter with LP and HP modes. 6 dB/octave slope. The simplest and cheapest filter in the factory.

  Signal flow:
    audio in -> one-pole IIR -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Cutoff frequency.
    Q: 0.3 - 9.9 (default 1.0). Ignored.
    Gain: -18 - 18 dB (default 0). Ignored.
    Smoothing: 0 - 1 s (default 0.01). Parameter interpolation time.
    Mode: LP, HP (default LP).
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    Gentle tone shaping, DC blocking (HP mode), control signal smoothing. When a steep rolloff is not needed and minimal CPU cost matters.

  See also:
    [alternative] filters.svf - second-order SVF for steeper rolloff
---

A first-order filter with lowpass and highpass modes, providing a gentle 6 dB/octave slope. This is the simplest and cheapest filter in the factory - one multiply-add per sample per channel.

![One Pole screenshot](/images/v2/reference/scriptnodes/filters/one_pole.png)

Useful for gentle tone shaping, DC blocking (HP mode at a low frequency), or smoothing control signals. When a steeper rolloff is needed, use [filters.svf]($SN.filters.svf$) or another second-order filter instead.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Cutoff frequency"
      range: "20 - 20000 Hz"
      default: "1000"
    Mode:
      desc: "LP (lowpass) or HP (highpass)"
      range: "LP / HP"
      default: "LP"
---

```
// filters.one_pole - first-order IIR filter
// audio in -> audio out

process(input) {
    lowpassed = a0 * input + b1 * previousOutput

    if Mode == LP:
        output = lowpassed
    else:
        output = input - lowpassed    // highpass = input minus lowpass
}
```

::

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - one_pole.LP.F1000
  - one_pole.HP.F1000
---
::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Cutoff frequency.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Not used. Present for interface consistency.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Not used. Present for interface consistency.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "Lowpass or highpass.", range: "LP / HP", default: "LP" }
      - { name: Smoothing, desc: "Interpolation time for frequency changes.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass.", range: "Off / On", default: "On" }
---
::

**See also:** $SN.filters.svf$ -- second-order SVF for steeper rolloff
