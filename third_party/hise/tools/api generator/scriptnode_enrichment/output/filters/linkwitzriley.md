---
title: Linkwitz-Riley
description: "A fourth-order Linkwitz-Riley (LR4) crossover filter with a 24 dB/octave slope."
factoryPath: filters.linkwitzriley
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/linkwitzriley.png
polyphonic: true
tags: [filters, linkwitz-riley, crossover, lowpass, highpass, allpass]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.svf", type: alternative, reason: "SVF for general filtering (not crossover use)" }
commonMistakes:
  - title: "Use two nodes for crossover bands"
    wrong: "Using a single linkwitzriley node and expecting to get both LP and HP outputs simultaneously"
    right: "Use two linkwitzriley nodes at the same frequency - one in LP mode, one in HP mode - to build a crossover."
    explanation: "Each node instance outputs only one band. To split a signal into low and high bands, place two nodes in parallel with matching frequency settings."
llmRef: |
  filters.linkwitzriley

  Fourth-order Linkwitz-Riley (LR4) crossover filter with 24 dB/oct slope. Computes both LP and HP internally, outputs one based on Mode. LP + HP sum to flat allpass (unity gain at crossover).

  Signal flow:
    audio in -> LR4 filter (both LP and HP computed) -> selected output

  CPU: low, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Crossover frequency.
    Q: 0.3 - 9.9 (default 1.0). Ignored - LR filters have fixed Q.
    Gain: -18 - 18 dB (default 0). Ignored.
    Smoothing: 0 - 1 s (default 0.01). Parameter interpolation time.
    Mode: LP, HP, AP (default LP). AP = LP + HP sum (flat).
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    Frequency-band splitting for multiband processing. Use two instances at the same frequency for a proper crossover. The LP and HP outputs sum to unity gain at the crossover point with no amplitude colouring.

  Common mistakes:
    Use two nodes for a crossover - one LP, one HP. A single node only outputs one band.

  See also:
    [alternative] filters.svf - general-purpose filter (not crossover)
---

A fourth-order Linkwitz-Riley (LR4) crossover filter with a 24 dB/octave slope. It computes both lowpass and highpass paths internally and selects the output based on the Mode parameter. When two instances are set to the same frequency - one in LP mode and one in HP mode - their outputs sum to the original signal with only phase shift and no amplitude colouring at the crossover point.

![Linkwitz-Riley screenshot](/images/v2/reference/scriptnodes/filters/linkwitzriley.png)

The Q and Gain parameters are ignored. Linkwitz-Riley filters have a fixed Q by design.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Crossover frequency"
      range: "20 - 20000 Hz"
      default: "1000"
    Mode:
      desc: "Output selection: LP, HP, or AP (LP + HP sum)"
      range: "LP / HP / AP"
      default: "LP"
  functions:
    lr4Filter:
      desc: "Fourth-order Linkwitz-Riley filter stage"
---

```
// filters.linkwitzriley - LR4 crossover filter
// audio in -> audio out

process(input) {
    lp = lr4Filter(input, Frequency, "lowpass")
    hp = lr4Filter(input, Frequency, "highpass")

    if Mode == LP:
        output = lp
    else if Mode == HP:
        output = hp
    else:
        output = lp + hp    // allpass (flat response)
}
```

::

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - linkwitzriley.LP.F1000
  - linkwitzriley.HP.F1000
  - linkwitzriley.Allpass.F1000
---
::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Crossover frequency. Set two instances to the same value for a matched crossover pair.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Not used. Linkwitz-Riley filters have fixed Q.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Not used.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "Selects which output to use. LP and HP are the two crossover bands. AP sums both for a flat allpass response.", range: "LP / HP / AP", default: "LP" }
      - { name: Smoothing, desc: "Interpolation time for frequency changes.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass.", range: "Off / On", default: "On" }
---
::

### Building a Crossover

Both the LP and HP processing paths run regardless of Mode, so AP mode has no additional CPU cost over LP or HP alone. To build a two-band crossover, place two linkwitzriley nodes in a [container.split]($SN.container.split$) with matching Frequency values. The first node in LP mode provides the low band; the second in HP mode provides the high band.

**See also:** $SN.filters.svf$ -- general-purpose filter for non-crossover use
