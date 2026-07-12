---
title: Linkwitz-Riley Filter
description: "A 4th-order (24 dB/octave) Linkwitz-Riley crossover filter with low-pass, high-pass, and allpass modes."
factoryPath: jdsp.jlinkwitzriley
factory: jdsp
polyphonic: false
tags: [jdsp, filter, crossover]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "filters.linkwitzriley", type: disambiguation, reason: "Deprecated alternative with stability issues when modulating frequency" }
  - { id: "template.freq_split3", type: companion, reason: "Pre-built multiband splitter using jlinkwitzriley nodes" }
commonMistakes:
  - title: "Use jlinkwitzriley instead of filters.linkwitzriley"
    wrong: "Using the deprecated filters.linkwitzriley node"
    right: "Use jdsp.jlinkwitzriley for stable frequency modulation and correct crossover behaviour."
    explanation: "The deprecated filters.linkwitzriley node has stability issues when modulating the cutoff frequency. The jlinkwitzriley node uses a more robust implementation."
llmRef: |
  jdsp.jlinkwitzriley

  A 4th-order (24 dB/octave) Linkwitz-Riley crossover filter. Three modes: low-pass, high-pass, and allpass. LP and HP outputs sum to unity gain with flat magnitude at the crossover frequency, making it ideal for crossover networks. Includes filter coefficient display. Monophonic.

  Signal flow:
    audio in -> 4th-order Linkwitz-Riley filter (LP/HP/AP) -> audio out

  CPU: low, monophonic

  Parameters:
    Frequency (20 - 20000 Hz, default 2000, log skew) - cutoff/crossover frequency
    Type (LP / HP / AP, default LP) - filter mode

  When to use:
    Crossover networks, multiband splitting, phase-aligned filtering. Use the freq_split templates for pre-configured multiband setups. Preferred over the deprecated filters.linkwitzriley.

  Common mistakes:
    Use jlinkwitzriley, not the deprecated filters.linkwitzriley.

  See also:
    disambiguation filters.linkwitzriley -- deprecated, has stability issues
    companion template.freq_split3 -- pre-built multiband splitter
---

A 4th-order (24 dB/octave) Linkwitz-Riley crossover filter with three modes: low-pass, high-pass, and allpass. Linkwitz-Riley filters are designed so that the low-pass and high-pass outputs sum to unity gain with a flat magnitude response at the crossover frequency, making them ideal for multiband splitting where the recombined signal must be transparent.

The allpass mode maintains the same phase characteristics as the LP/HP modes without attenuating any frequencies, which is useful for maintaining phase coherence when recombining bands in a multiband setup.

This node includes a filter coefficient display that shows the frequency response curve in the scriptnode UI. The frequency parameter is validated to stay within the 20-20000 Hz range, ensuring stable operation during modulation.

> [!Tip:Use freq_split templates for multiband setups] Setting up multiple Linkwitz-Riley filters with the correct mode settings and parameter connections is complex. Use the [freq_split]($SN.template.freq_split3$) templates which create a pre-configured multiband container automatically.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Cutoff/crossover frequency"
      range: "20 - 20000 Hz"
      default: "2000"
    Type:
      desc: "Filter mode: low-pass, high-pass, or allpass"
      range: "LP / HP / AP"
      default: "LP"
  functions:
    linkwitzRileyFilter:
      desc: "4th-order (24 dB/oct) Linkwitz-Riley filter"
---

```
// jdsp.jlinkwitzriley - crossover filter
// audio in -> audio out

process(input) {
    output = linkwitzRileyFilter(input, Frequency, Type)
    // LP + HP sum to unity at crossover frequency
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Filter
    params:
      - { name: Frequency, desc: "The cutoff frequency of the filter. At this frequency, the LP and HP modes each attenuate by 6 dB, and their outputs sum to 0 dB.", range: "20 - 20000 Hz", default: "2000" }
      - { name: Type, desc: "The filter mode. LP passes frequencies below the cutoff. HP passes frequencies above. AP passes all frequencies but applies the same phase shift as the LP/HP modes.", range: "LP / HP / AP", default: "LP" }
---
::

**See also:** [$SN.filters.linkwitzriley$]($SN.filters.linkwitzriley$) -- deprecated alternative with stability issues when modulating frequency, [$SN.template.freq_split3$]($SN.template.freq_split3$) -- pre-built multiband splitter using jlinkwitzriley nodes
