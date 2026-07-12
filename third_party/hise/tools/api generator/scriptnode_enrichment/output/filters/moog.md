---
title: Moog
description: "A Moog-style transistor ladder lowpass filter with a 24 dB/octave slope."
factoryPath: filters.moog
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/moog.png
polyphonic: true
tags: [filters, moog, ladder, lowpass, analog, resonance]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.ladder", type: disambiguation, reason: "Simpler ladder lowpass with lighter CPU cost" }
  - { id: "filters.svf", type: alternative, reason: "SVF with multiple filter types and better modulation stability" }
  - { id: "PolyphonicFilter", type: module, reason: "Module-tree filter -- scriptnode offers individual filter types as separate nodes" }
commonMistakes:
  - title: "Mode only affects display curve"
    wrong: "Switching the Mode parameter expecting different filter slopes"
    right: "The Mode parameter only changes the filter display curve, not the audio processing."
    explanation: "Despite showing One Pole, Two Poles, and Four Poles options, the Moog filter always processes as a full 4-pole (24 dB/oct) ladder. The Mode parameter affects only the frequency response visualisation."
llmRef: |
  filters.moog

  Moog-style transistor ladder lowpass filter with 24 dB/oct slope. Analog-modelled with resonance feedback that can approach self-oscillation at high Q. Always processes as 4-pole regardless of Mode setting.

  Signal flow:
    audio in -> 4-stage ladder with feedback -> audio out

  CPU: low, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Cutoff frequency.
    Q: 0.3 - 9.9 (default 1.0). Resonance feedback. High values approach self-oscillation.
    Gain: -18 - 18 dB (default 0). Ignored.
    Smoothing: 0 - 1 s (default 0.01). Parameter interpolation time.
    Mode: One Pole, Two Poles, Four Poles (default One Pole). Display only - does not affect processing.
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    Classic analog-style lowpass with character. Choose over filters.ladder when a warmer, more coloured sound is desired. Not suited for EQ or multimode filtering.

  Common mistakes:
    Mode parameter is display-only - processing is always 24 dB/oct regardless of Mode setting.

  See also:
    [disambiguation] filters.ladder - simpler ladder lowpass, lighter CPU
    [alternative] filters.svf - multimode SVF with better modulation stability
    [module] PolyphonicFilter - module-tree filter -- scriptnode offers individual filter types as separate nodes
---

A Moog-style transistor ladder lowpass filter with a 24 dB/octave slope. The analog-modelled design includes input history mixing and specific scaling constants that give it a warmer, more coloured character than the simpler [filters.ladder]($SN.filters.ladder$).

![Moog screenshot](/images/v2/reference/scriptnodes/filters/moog.png)

The Q parameter drives resonance feedback that can approach self-oscillation at high values. This filter is always a 4-pole lowpass regardless of the Mode setting.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Cutoff frequency"
      range: "20 - 20000 Hz"
      default: "1000"
    Q:
      desc: "Resonance feedback amount"
      range: "0.3 - 9.9"
      default: "1.0"
  functions:
    feedbackSubtract:
      desc: "Subtracts output feedback from input to create resonance"
    ladderStage:
      desc: "One-pole filter stage with input history mixing"
---

```
// filters.moog - Moog transistor ladder lowpass
// audio in -> audio out

process(input) {
    feedback = stage4Output * resonance(Q)
    signal = feedbackSubtract(input, feedback)

    stage1 = ladderStage(signal, history1, state1)
    stage2 = ladderStage(stage1,  history2, state2)
    stage3 = ladderStage(stage2,  history3, state3)
    stage4 = ladderStage(stage3,  history4, state4)

    output = 2.0 * stage4    // always 4-pole output
}
```

::

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - moog.FourPole.Q05.F1000
  - moog.FourPole.Q10.F1000
  - moog.FourPole.Q50.F1000
---
::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Cutoff frequency of the lowpass filter.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Resonance feedback. Higher values produce a stronger resonant peak at the cutoff frequency, approaching self-oscillation.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Not used. Present for interface consistency.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "Display only. Changes the filter response visualisation but does not affect audio processing. The filter always operates as a 4-pole (24 dB/oct) lowpass.", range: "One Pole / Two Poles / Four Poles", default: "One Pole" }
      - { name: Smoothing, desc: "Interpolation time for parameter changes.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass.", range: "Off / On", default: "On" }
---
::

**See also:** $SN.filters.ladder$ -- simpler ladder lowpass with lighter CPU cost, $SN.filters.svf$ -- multimode SVF with better modulation stability, $MODULES.PolyphonicFilter$ -- module-tree filter -- scriptnode offers individual filter types as separate nodes
