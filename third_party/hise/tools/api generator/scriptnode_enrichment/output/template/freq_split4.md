---
title: Freq Split 4
description: "A 4-band frequency crossover using phase-coherent Linkwitz-Riley filters."
factoryPath: template.freq_split4
factory: template
polyphonic: false
tags: [template, crossover, multiband]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "template.freq_split2", type: alternative, reason: "2-band crossover when fewer bands are sufficient" }
  - { id: "template.freq_split3", type: alternative, reason: "3-band crossover for lighter CPU usage" }
  - { id: "template.freq_split5", type: alternative, reason: "5-band crossover for maximum frequency separation" }
  - { id: "container.split", type: companion, reason: "The underlying parallel container used internally" }
  - { id: "jdsp.jlinkwitzriley", type: companion, reason: "The Linkwitz-Riley crossover filter used internally" }
commonMistakes:
  - title: "Forgetting to replace dummy placeholders"
    wrong: "Inserting a freq_split4 and leaving the default math.mul nodes in each band"
    right: "Replace or supplement each band's math.mul placeholder with per-band processing (compression, EQ, saturation, etc.)."
    explanation: "The template ships with math.mul nodes set to 1.0 (passthrough) as placeholders. Without replacing them, the crossover splits and recombines the signal with no audible effect."
  - title: "Setting crossover frequencies in wrong order"
    wrong: "Setting Band 2 lower than Band 1, or Band 3 lower than Band 2"
    right: "Keep crossover frequencies in ascending order: Band 1 < Band 2 < Band 3."
    explanation: "The crossover parameters define boundaries between adjacent bands. If they overlap or reverse, the band filters will produce unexpected frequency responses."
llmRef: |
  template.freq_split4

  A 4-band frequency crossover that splits audio into four bands using Linkwitz-Riley 4th-order filters with allpass phase alignment. Each band contains a placeholder node for user processing. The bands sum back together with flat magnitude response.

  Signal flow:
    input --split--> [LP1, AP2, AP3] --> user processing (low) -----\
    input --split--> [HP1, LP2, AP3] --> user processing (low-mid) --+--> sum --> output
    input --split--> [AP1, HP2, LP3] --> user processing (high-mid) -/
    input --split--> [AP1, AP2, HP3] --> user processing (high) ----/

  CPU: low, monophonic
    12 Linkwitz-Riley 4th-order filters (4 bands x 3 crossover points).

  Parameters:
    Band 1: crossover 1 frequency (20 - 20000 Hz, log, default 38 Hz)
    Band 2: crossover 2 frequency (20 - 20000 Hz, log, default 391 Hz)
    Band 3: crossover 3 frequency (20 - 20000 Hz, log, default 2186 Hz)

  When to use:
    4-band multiband processing with independent control over sub-bass, low-mids, high-mids, and treble.

  Common mistakes:
    Leaving the math.mul placeholders at default produces no audible effect - replace them with per-band processing.
    Crossover frequencies must be in ascending order (Band 1 < Band 2 < Band 3).

  See also:
    [alternative] template.freq_split2 -- 2-band crossover
    [alternative] template.freq_split3 -- 3-band crossover
    [alternative] template.freq_split5 -- 5-band crossover
    [companion] container.split -- underlying parallel container
    [companion] jdsp.jlinkwitzriley -- crossover filter used internally
---

A pre-built 4-band frequency crossover network. It splits the input signal into four frequency bands using Linkwitz-Riley 4th-order filters, processes each band independently, and sums the results. Each band contains three filters - one per crossover point - combining lowpass, highpass, and allpass responses to isolate the correct frequency range while maintaining phase coherence across all bands.

Each band contains a placeholder node that you replace with your own processing chain. The two interior bands use highpass and lowpass combinations to create bandpass responses between adjacent crossover frequencies. For fewer bands use [freq_split2]($SN.template.freq_split2$) or [freq_split3]($SN.template.freq_split3$); for more, use [freq_split5]($SN.template.freq_split5$).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Band 1:
      desc: "Crossover frequency between the lowest and low-mid bands"
      range: "20 - 20000 Hz"
      default: "38 Hz"
    Band 2:
      desc: "Crossover frequency between the low-mid and high-mid bands"
      range: "20 - 20000 Hz"
      default: "391 Hz"
    Band 3:
      desc: "Crossover frequency between the high-mid and highest bands"
      range: "20 - 20000 Hz"
      default: "2186 Hz"
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
// template.freq_split4 - 4-band frequency crossover
// audio in -> audio out

process(input) {
    // Low band: LP at crossover 1, AP at crossovers 2 and 3
    low = lowpass(input, Band 1)
    low = allpass(low, Band 2)
    low = allpass(low, Band 3)
    low = userProcessing(low)

    // Low-mid band: HP at crossover 1, LP at crossover 2, AP at crossover 3
    lowMid = highpass(input, Band 1)
    lowMid = lowpass(lowMid, Band 2)
    lowMid = allpass(lowMid, Band 3)
    lowMid = userProcessing(lowMid)

    // High-mid band: AP at crossover 1, HP at crossover 2, LP at crossover 3
    highMid = allpass(input, Band 1)
    highMid = highpass(highMid, Band 2)
    highMid = lowpass(highMid, Band 3)
    highMid = userProcessing(highMid)

    // High band: AP at crossovers 1 and 2, HP at crossover 3
    high = allpass(input, Band 1)
    high = allpass(high, Band 2)
    high = highpass(high, Band 3)
    high = userProcessing(high)

    output = sum(low, lowMid, highMid, high)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Crossover
    params:
      - { name: "Band 1", desc: "Crossover frequency between the lowest and low-mid bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "38.2" }
      - { name: "Band 2", desc: "Crossover frequency between the low-mid and high-mid bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "391.3" }
      - { name: "Band 3", desc: "Crossover frequency between the high-mid and highest bands. Uses logarithmic scaling for natural frequency control.", range: "20 - 20000 Hz", default: "2185.9" }
---
::

### Setup

The placeholder nodes in each band are [math.mul]($SN.math.mul$) set to 1.0 (passthrough) -- replace or supplement these with your per-band processing. Each band contains 3 Linkwitz-Riley filters (12 total). The allpass filters at non-boundary crossover points ensure all bands share the same total phase shift, preserving flat magnitude response when summed. Keep crossover frequencies in ascending order: Band 1 < Band 2 < Band 3 -- reversing them produces unexpected filter behaviour.

**See also:** $SN.template.freq_split2$ -- 2-band crossover when fewer bands are sufficient, $SN.template.freq_split3$ -- 3-band crossover for lighter CPU usage, $SN.template.freq_split5$ -- 5-band crossover for maximum frequency separation, $SN.container.split$ -- the underlying parallel container used internally, $SN.jdsp.jlinkwitzriley$ -- the Linkwitz-Riley crossover filter used internally
