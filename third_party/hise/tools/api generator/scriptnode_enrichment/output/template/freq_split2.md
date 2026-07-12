---
title: Freq Split 2
description: "A 2-band frequency crossover using phase-coherent Linkwitz-Riley filters."
factoryPath: template.freq_split2
factory: template
polyphonic: false
tags: [template, crossover, multiband]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "template.freq_split3", type: alternative, reason: "3-band crossover for more frequency separation" }
  - { id: "template.freq_split4", type: alternative, reason: "4-band crossover" }
  - { id: "template.freq_split5", type: alternative, reason: "5-band crossover for maximum frequency separation" }
  - { id: "container.split", type: companion, reason: "The underlying parallel container used internally" }
  - { id: "jdsp.jlinkwitzriley", type: companion, reason: "The Linkwitz-Riley crossover filter used internally" }
commonMistakes:
  - title: "Forgetting to replace dummy placeholders"
    wrong: "Inserting a freq_split2 and leaving the default math.mul nodes in each band"
    right: "Replace or supplement each band's math.mul placeholder with per-band processing (compression, EQ, saturation, etc.)."
    explanation: "The template ships with math.mul nodes set to 1.0 (passthrough) as placeholders. Without replacing them, the crossover splits and recombines the signal with no audible effect."
  - title: "Expecting more than two bands"
    wrong: "Trying to add a third band to freq_split2"
    right: "Use template.freq_split3 or higher for more bands."
    explanation: "The freq_splitN variants have a fixed band count. Choose the variant that matches the number of bands you need."
llmRef: |
  template.freq_split2

  A 2-band frequency crossover that splits audio into low and high bands using Linkwitz-Riley 4th-order filters. Each band contains a placeholder node for user processing. The bands sum back together with flat magnitude response.

  Signal flow:
    input --split--> [LP at Band 1] --> user processing (low) --\
    input --split--> [HP at Band 1] --> user processing (high) --+--> sum --> output

  CPU: low, monophonic
    2 Linkwitz-Riley 4th-order filters (each is two biquad sections).

  Parameters:
    Band 1: crossover frequency between low and high bands (20 - 20000 Hz, log, default 188 Hz)

  When to use:
    Simple low/high multiband processing such as applying different compression, saturation, or EQ to bass and treble independently.

  Common mistakes:
    Leaving the math.mul placeholders at default produces no audible effect - replace them with per-band processing.

  See also:
    [alternative] template.freq_split3 -- 3-band crossover
    [alternative] template.freq_split4 -- 4-band crossover
    [alternative] template.freq_split5 -- 5-band crossover
    [companion] container.split -- underlying parallel container
    [companion] jdsp.jlinkwitzriley -- crossover filter used internally
---

A pre-built 2-band frequency crossover network. It splits the input signal into low and high frequency bands using Linkwitz-Riley 4th-order filters, processes each band independently, and sums the results. The Linkwitz-Riley design guarantees flat magnitude response at the crossover point, so the bands recombine transparently when no per-band processing is applied.

Each band contains a placeholder node that you replace with your own processing chain. Typical uses include applying different compression, saturation, or EQ to bass and treble independently. For more than two bands, use [freq_split3]($SN.template.freq_split3$) through [freq_split5]($SN.template.freq_split5$).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Band 1:
      desc: "Crossover frequency between the low and high bands"
      range: "20 - 20000 Hz"
      default: "188 Hz"
  functions:
    lowpass:
      desc: "Linkwitz-Riley 4th-order lowpass filter at the crossover frequency"
    highpass:
      desc: "Linkwitz-Riley 4th-order highpass filter at the crossover frequency"
    sum:
      desc: "Adds the band outputs to reconstruct the full spectrum"
---

```
// template.freq_split2 - 2-band frequency crossover
// audio in -> audio out

process(input) {
    // Low band
    low = lowpass(input, Band 1)
    low = userProcessing(low)

    // High band
    high = highpass(input, Band 1)
    high = userProcessing(high)

    output = sum(low, high)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Crossover
    params:
      - { name: "Band 1", desc: "Crossover frequency between the low and high bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "188.0" }
---
::

### Setup

The placeholder nodes in each band are [math.mul]($SN.math.mul$) set to 1.0 (passthrough) -- replace or supplement these with your per-band processing. At the crossover frequency, both filters are at -6 dB. Their in-phase sum equals 0 dB, preserving unity gain. This is the simplest freq_split variant; if you need finer frequency separation, choose a higher band count variant.

**See also:** $SN.template.freq_split3$ -- 3-band crossover for more frequency separation, $SN.template.freq_split4$ -- 4-band crossover, $SN.template.freq_split5$ -- 5-band crossover for maximum frequency separation, $SN.container.split$ -- the underlying parallel container used internally, $SN.jdsp.jlinkwitzriley$ -- the Linkwitz-Riley crossover filter used internally
