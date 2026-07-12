---
title: Freq Split 5
description: "A 5-band frequency crossover using phase-coherent Linkwitz-Riley filters."
factoryPath: template.freq_split5
factory: template
polyphonic: false
tags: [template, crossover, multiband]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "template.freq_split2", type: alternative, reason: "2-band crossover when fewer bands are sufficient" }
  - { id: "template.freq_split3", type: alternative, reason: "3-band crossover for lighter CPU usage" }
  - { id: "template.freq_split4", type: alternative, reason: "4-band crossover for lighter CPU usage" }
  - { id: "container.split", type: companion, reason: "The underlying parallel container used internally" }
  - { id: "jdsp.jlinkwitzriley", type: companion, reason: "The Linkwitz-Riley crossover filter used internally" }
commonMistakes:
  - title: "Forgetting to replace dummy placeholders"
    wrong: "Inserting a freq_split5 and leaving the default math.mul nodes in each band"
    right: "Replace or supplement each band's math.mul placeholder with per-band processing (compression, EQ, saturation, etc.)."
    explanation: "The template ships with math.mul nodes set to 1.0 (passthrough) as placeholders. Without replacing them, the crossover splits and recombines the signal with no audible effect."
  - title: "Setting crossover frequencies in wrong order"
    wrong: "Setting crossover frequencies out of ascending order"
    right: "Keep crossover frequencies in ascending order: Band 1 < Band 2 < Band 3 < Band 4."
    explanation: "The crossover parameters define boundaries between adjacent bands. If they overlap or reverse, the band filters will produce unexpected frequency responses."
llmRef: |
  template.freq_split5

  A 5-band frequency crossover that splits audio into five bands using Linkwitz-Riley 4th-order filters with allpass phase alignment. Each band contains a placeholder node for user processing. The bands sum back together with flat magnitude response. This is the largest freq_split variant.

  Signal flow:
    input --split--> [LP1, AP2, AP3, AP4] --> user processing (low) -----\
    input --split--> [HP1, LP2, AP3, AP4] --> user processing (low-mid) --\
    input --split--> [AP1, HP2, LP3, AP4] --> user processing (mid) -------+--> sum --> output
    input --split--> [AP1, AP2, HP3, LP4] --> user processing (high-mid) --/
    input --split--> [AP1, AP2, AP3, HP4] --> user processing (high) -----/

  CPU: medium, monophonic
    20 Linkwitz-Riley 4th-order filters (5 bands x 4 crossover points).

  Parameters:
    Band 1: crossover 1 frequency (20 - 20000 Hz, log, default 28 Hz)
    Band 2: crossover 2 frequency (20 - 20000 Hz, log, default 188 Hz)
    Band 3: crossover 3 frequency (20 - 20000 Hz, log, default 1000 Hz)
    Band 4: crossover 4 frequency (20 - 20000 Hz, log, default 3445 Hz)

  When to use:
    5-band multiband processing with fine-grained control over sub-bass, bass, mids, presence, and treble. Use when 4 bands are not enough.

  Common mistakes:
    Leaving the math.mul placeholders at default produces no audible effect - replace them with per-band processing.
    Crossover frequencies must be in ascending order (Band 1 < Band 2 < Band 3 < Band 4).

  See also:
    [alternative] template.freq_split2 -- 2-band crossover
    [alternative] template.freq_split3 -- 3-band crossover
    [alternative] template.freq_split4 -- 4-band crossover
    [companion] container.split -- underlying parallel container
    [companion] jdsp.jlinkwitzriley -- crossover filter used internally
---

A pre-built 5-band frequency crossover network - the largest variant in the freq_split family. It splits the input signal into five frequency bands using Linkwitz-Riley 4th-order filters, processes each band independently, and sums the results. Each band contains four filters - one per crossover point - combining lowpass, highpass, and allpass responses to isolate the correct frequency range while maintaining phase coherence across all bands.

Each band contains a placeholder node that you replace with your own processing chain. The three interior bands use highpass and lowpass combinations to create bandpass responses between adjacent crossover frequencies. This variant uses 20 filters in total, so consider whether [freq_split3]($SN.template.freq_split3$) or [freq_split4]($SN.template.freq_split4$) provides sufficient separation before choosing 5 bands. For a simpler 2-band split, use [freq_split2]($SN.template.freq_split2$).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Band 1:
      desc: "Crossover frequency between the lowest and low-mid bands"
      range: "20 - 20000 Hz"
      default: "28 Hz"
    Band 2:
      desc: "Crossover frequency between the low-mid and mid bands"
      range: "20 - 20000 Hz"
      default: "188 Hz"
    Band 3:
      desc: "Crossover frequency between the mid and high-mid bands"
      range: "20 - 20000 Hz"
      default: "1000 Hz"
    Band 4:
      desc: "Crossover frequency between the high-mid and highest bands"
      range: "20 - 20000 Hz"
      default: "3445 Hz"
  functions:
    lowpass:
      desc: "Linkwitz-Riley 4th-order lowpass filter at the given crossover frequency"
    highpass:
      desc: "Linkwitz-Riley 4th-order highpass filter at the given crossover frequency"
    allpass:
      desc: "Linkwitz-Riley 4th-order allpass filter for phase alignment at the given crossover frequency"
    sum:
      desc: "Adds all band outputs to reconstruct the full spectrum"
---

```
// template.freq_split5 - 5-band frequency crossover
// audio in -> audio out

process(input) {
    // Low band: LP at crossover 1, AP at crossovers 2, 3, and 4
    low = lowpass(input, Band 1)
    low = allpass(low, Band 2)
    low = allpass(low, Band 3)
    low = allpass(low, Band 4)
    low = userProcessing(low)

    // Low-mid band: HP at crossover 1, LP at crossover 2, AP at crossovers 3 and 4
    lowMid = highpass(input, Band 1)
    lowMid = lowpass(lowMid, Band 2)
    lowMid = allpass(lowMid, Band 3)
    lowMid = allpass(lowMid, Band 4)
    lowMid = userProcessing(lowMid)

    // Mid band: AP at crossover 1, HP at crossover 2, LP at crossover 3, AP at crossover 4
    mid = allpass(input, Band 1)
    mid = highpass(mid, Band 2)
    mid = lowpass(mid, Band 3)
    mid = allpass(mid, Band 4)
    mid = userProcessing(mid)

    // High-mid band: AP at crossovers 1 and 2, HP at crossover 3, LP at crossover 4
    highMid = allpass(input, Band 1)
    highMid = allpass(highMid, Band 2)
    highMid = highpass(highMid, Band 3)
    highMid = lowpass(highMid, Band 4)
    highMid = userProcessing(highMid)

    // High band: AP at crossovers 1, 2, and 3, HP at crossover 4
    high = allpass(input, Band 1)
    high = allpass(high, Band 2)
    high = allpass(high, Band 3)
    high = highpass(high, Band 4)
    high = userProcessing(high)

    output = sum(low, lowMid, mid, highMid, high)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Crossover
    params:
      - { name: "Band 1", desc: "Crossover frequency between the lowest and low-mid bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "28.2" }
      - { name: "Band 2", desc: "Crossover frequency between the low-mid and mid bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "188.0" }
      - { name: "Band 3", desc: "Crossover frequency between the mid and high-mid bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "1000.0" }
      - { name: "Band 4", desc: "Crossover frequency between the high-mid and highest bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "3445.0" }
---
::

### Setup

The placeholder nodes in each band are [math.mul]($SN.math.mul$) set to 1.0 (passthrough) -- replace or supplement these with your per-band processing. Each band contains 4 Linkwitz-Riley filters (20 total), making this the most CPU-intensive freq_split variant. The allpass filters are necessary for phase coherence but add overhead compared to simpler crossover designs. Keep crossover frequencies in ascending order: Band 1 < Band 2 < Band 3 < Band 4 -- reversing them produces unexpected filter behaviour. The default frequencies are spaced logarithmically across the audible range, providing roughly equal perceptual bandwidth per band.

**See also:** $SN.template.freq_split2$ -- 2-band crossover when fewer bands are sufficient, $SN.template.freq_split3$ -- 3-band crossover for lighter CPU usage, $SN.template.freq_split4$ -- 4-band crossover for lighter CPU usage, $SN.container.split$ -- the underlying parallel container used internally, $SN.jdsp.jlinkwitzriley$ -- the Linkwitz-Riley crossover filter used internally
