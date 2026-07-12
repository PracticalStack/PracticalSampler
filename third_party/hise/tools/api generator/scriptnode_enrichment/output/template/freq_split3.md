---
title: Freq Split 3
description: "A 3-band frequency crossover using phase-coherent Linkwitz-Riley filters."
factoryPath: template.freq_split3
factory: template
polyphonic: false
tags: [template, crossover, multiband]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "template.freq_split2", type: alternative, reason: "2-band crossover when fewer bands are sufficient" }
  - { id: "template.freq_split4", type: alternative, reason: "4-band crossover for finer frequency separation" }
  - { id: "template.freq_split5", type: alternative, reason: "5-band crossover for maximum frequency separation" }
  - { id: "container.split", type: companion, reason: "The underlying parallel container used internally" }
  - { id: "jdsp.jlinkwitzriley", type: companion, reason: "The Linkwitz-Riley crossover filter used internally" }
commonMistakes:
  - title: "Forgetting to replace dummy placeholders"
    wrong: "Inserting a freq_split3 and leaving the default math.mul nodes in each band"
    right: "Replace or supplement each band's math.mul placeholder with per-band processing (compression, EQ, saturation, etc.)."
    explanation: "The template ships with math.mul nodes set to 1.0 (passthrough) as placeholders. Without replacing them, the crossover splits and recombines the signal with no audible effect."
  - title: "Setting crossover frequencies in wrong order"
    wrong: "Setting Band 2 lower than Band 1"
    right: "Keep crossover frequencies in ascending order: Band 1 < Band 2."
    explanation: "The crossover parameters define boundaries between adjacent bands. If they overlap or reverse, the band filters will produce unexpected frequency responses."
llmRef: |
  template.freq_split3

  A 3-band frequency crossover that splits audio into low, mid, and high bands using Linkwitz-Riley 4th-order filters with allpass phase alignment. Each band contains a placeholder node for user processing. The bands sum back together with flat magnitude response.

  Signal flow:
    input --split--> [LP1, AP2] --> user processing (low) --\
    input --split--> [HP1, LP2] --> user processing (mid) ---+--> sum --> output
    input --split--> [AP1, HP2] --> user processing (high) --/

  CPU: low, monophonic
    6 Linkwitz-Riley 4th-order filters (3 bands x 2 crossover points).

  Parameters:
    Band 1: low-mid crossover frequency (20 - 20000 Hz, log, default 68 Hz)
    Band 2: mid-high crossover frequency (20 - 20000 Hz, log, default 1000 Hz)

  When to use:
    3-band multiband processing such as independent compression, saturation, or EQ on bass, mids, and treble.

  Common mistakes:
    Leaving the math.mul placeholders at default produces no audible effect - replace them with per-band processing.
    Setting Band 2 below Band 1 produces unexpected filter responses.

  See also:
    [alternative] template.freq_split2 -- 2-band crossover
    [alternative] template.freq_split4 -- 4-band crossover
    [alternative] template.freq_split5 -- 5-band crossover
    [companion] container.split -- underlying parallel container
    [companion] jdsp.jlinkwitzriley -- crossover filter used internally
---

![3-band crossover diagram](/images/custom/scriptnode/lr4_3.png)

A pre-built 3-band frequency crossover network. It splits the input signal into low, mid, and high frequency bands using Linkwitz-Riley 4th-order filters, processes each band independently, and sums the results. Each band contains two filters - one per crossover point - plus allpass filters for phase alignment across all bands. The Linkwitz-Riley design guarantees flat magnitude response when the bands are recombined.

Each band contains a placeholder node that you replace with your own processing chain. The middle band uses a combination of highpass and lowpass filters to create a bandpass response between the two crossover frequencies. For fewer bands use [freq_split2]($SN.template.freq_split2$); for more, use [freq_split4]($SN.template.freq_split4$) or [freq_split5]($SN.template.freq_split5$).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Band 1:
      desc: "Crossover frequency between the low and mid bands"
      range: "20 - 20000 Hz"
      default: "68 Hz"
    Band 2:
      desc: "Crossover frequency between the mid and high bands"
      range: "20 - 20000 Hz"
      default: "1000 Hz"
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
// template.freq_split3 - 3-band frequency crossover
// audio in -> audio out

process(input) {
    // Low band: LP at crossover 1, AP at crossover 2
    low = lowpass(input, Band 1)
    low = allpass(low, Band 2)
    low = userProcessing(low)

    // Mid band: HP at crossover 1, LP at crossover 2
    mid = highpass(input, Band 1)
    mid = lowpass(mid, Band 2)
    mid = userProcessing(mid)

    // High band: AP at crossover 1, HP at crossover 2
    high = allpass(input, Band 1)
    high = highpass(high, Band 2)
    high = userProcessing(high)

    output = sum(low, mid, high)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Crossover
    params:
      - { name: "Band 1", desc: "Crossover frequency between the low and mid bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "68.0" }
      - { name: "Band 2", desc: "Crossover frequency between the mid and high bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "1000.0" }
---
::

### Setup

The placeholder nodes in each band are [math.mul]($SN.math.mul$) set to 1.0 (passthrough) -- replace or supplement these with your per-band processing. Allpass filters in the low and high bands match the phase shift introduced by the opposite crossover point, maintaining flat magnitude response across all three bands. Keep Band 1 below Band 2 -- reversing the crossover frequencies produces unexpected filter behaviour.

**See also:** $SN.template.freq_split2$ -- 2-band crossover when fewer bands are sufficient, $SN.template.freq_split4$ -- 4-band crossover for finer frequency separation, $SN.template.freq_split5$ -- 5-band crossover for maximum frequency separation, $SN.container.split$ -- the underlying parallel container used internally, $SN.jdsp.jlinkwitzriley$ -- the Linkwitz-Riley crossover filter used internally
