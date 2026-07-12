---
title: Allpass
description: "A six-stage cascaded allpass chain with feedback, producing phaser-style notches when the allpass output is mixed with the dry input."
factoryPath: filters.allpass
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/allpass.png
polyphonic: true
tags: [filters, allpass, phaser, phase-shift, effect]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.svf", type: disambiguation, reason: "SVF Allpass mode for true phase-only allpass (no amplitude notches)" }
commonMistakes:
  - title: "Allpass creates amplitude notches"
    wrong: "Using filters.allpass for phase correction expecting flat amplitude response"
    right: "Use filters.svf in Allpass mode (mode 4) for a true allpass with flat amplitude."
    explanation: "filters.allpass is a 6-stage cascaded allpass chain with feedback that produces phaser-style amplitude notches. It is an effect, not a utility allpass. For phase-only processing, use the SVF's Allpass mode."
llmRef: |
  filters.allpass

  Six-stage cascaded allpass chain with feedback, producing phaser-style notches. Mixes dry signal with the allpass output. Despite the name, this is a phaser effect, not a flat-amplitude allpass.

  Signal flow:
    audio in -> 6 cascaded allpass stages with feedback -> mixed with dry input -> audio out

  CPU: low, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Centre frequency for the allpass delay.
    Q: 0.3 - 9.9 (default 1.0). Feedback amount (0 = no feedback, max = strong resonance).
    Gain: -18 - 18 dB (default 0). Ignored.
    Smoothing: 0 - 1 s (default 0.01). Parameter interpolation time.
    Mode: AllPass only (default AllPass).
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    Phaser-style effects. Modulate Frequency with an LFO for classic phaser sweeps. Not for phase correction - use filters.svf Allpass mode instead.

  Common mistakes:
    This is a phaser effect, not a true allpass. Use filters.svf Allpass mode for phase-only processing.

  See also:
    [disambiguation] filters.svf - SVF Allpass mode for true phase-only allpass
---

A six-stage cascaded allpass chain with feedback, producing phaser-style notches when the allpass output is mixed with the dry input. Despite the name, this is a phaser effect that creates amplitude notches - not a flat-amplitude allpass filter.

![Allpass screenshot](/images/v2/reference/scriptnodes/filters/allpass.png)

The Frequency parameter sets the centre frequency of the allpass delay stages, and Q controls the feedback amount. At minimum Q, no feedback occurs and the effect is subtle. At high Q, sharp resonant notches appear. Modulate Frequency with an LFO for classic phaser sweeps.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Centre frequency for the allpass delay"
      range: "20 - 20000 Hz"
      default: "1000"
    Q:
      desc: "Feedback amount (higher = sharper notches)"
      range: "0.3 - 9.9"
      default: "1.0"
  functions:
    allpassStage:
      desc: "First-order allpass delay stage"
---

```
// filters.allpass - phaser-style allpass chain
// audio in -> audio out

process(input) {
    signal = input + previousOutput * feedback(Q)

    // 6 cascaded allpass stages at Frequency
    signal = allpassStage(signal, Frequency)    // x6
    signal = allpassStage(signal, Frequency)
    signal = allpassStage(signal, Frequency)
    signal = allpassStage(signal, Frequency)
    signal = allpassStage(signal, Frequency)
    signal = allpassStage(signal, Frequency)

    previousOutput = signal
    output = input + signal    // dry + wet mix
}
```

::

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - allpass.Phaser.Q05.F1000
  - allpass.Phaser.Q10.F1000
  - allpass.Phaser.Q50.F1000
---
::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Centre frequency for the allpass delay stages. Determines where the phaser notches appear.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Feedback amount. At minimum (0.3), no feedback occurs. At maximum (9.9), strong resonant peaks form near self-oscillation.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Not used. Present for interface consistency.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "Fixed to AllPass. Only one mode is available.", range: "AllPass", default: "AllPass" }
      - { name: Smoothing, desc: "Interpolation time for parameter changes.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass.", range: "Off / On", default: "On" }
---
::

**See also:** $SN.filters.svf$ -- SVF Allpass mode for true phase-only allpass (no amplitude notches)
