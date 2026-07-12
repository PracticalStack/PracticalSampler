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
    explanation: "Deprecated filters.linkwitzriley node has stability issues when modulating cutoff frequency. jlinkwitzriley node uses more robust implementation."
llmRef: |
  jdsp.jlinkwitzriley

  4th-order (24 dB/octave) Linkwitz-Riley crossover filter. Three modes: low-pass, high-pass, allpass. LP and HP outputs sum to unity gain with flat magnitude at crossover frequency, making it ideal for crossover networks. Includes filter coefficient display. Monophonic.

  Signal flow:
    audio in -> 4th-order Linkwitz-Riley filter (LP/HP/AP) -> audio out

  CPU: low, monophonic

  Parameters:
    Frequency (20 - 20000 Hz, default 2000, log skew) - cutoff/crossover frequency
    Type (LP / HP / AP, default LP) - filter mode

  When to use:
    Crossover networks, multiband splitting, phase-aligned filtering. Use freq_split templates for pre-configured multiband setups. Preferred over deprecated filters.linkwitzriley.

  Common mistakes:
    Use jlinkwitzriley, not deprecated filters.linkwitzriley.

  See also:
    disambiguation filters.linkwitzriley -- deprecated, has stability issues
    companion template.freq_split3 -- pre-built multiband splitter
---

4th-order (24 dB/octave) Linkwitz-Riley crossover filter with three modes: low-pass, high-pass, allpass. Linkwitz-Riley filters are designed so low-pass and high-pass outputs sum to unity gain with flat magnitude response at crossover frequency, making them ideal for multiband splitting where recombined signal must be transparent.

Allpass mode maintains same phase characteristics as LP/HP modes without attenuating any frequencies, which is useful for maintaining phase coherence when recombining bands in multiband setup.

This node includes filter coefficient display showing frequency response curve in scriptnode UI. Frequency parameter is validated to stay within 20-20000 Hz range, ensuring stable operation during modulation.

> [!Tip:Use freq_split templates for multiband setups] Setting up multiple Linkwitz-Riley filters with correct mode settings and parameter connections is complex. Use [freq_split]($SN.template.freq_split3$) templates which create pre-configured multiband container automatically.

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
      - { name: Frequency, desc: "Cutoff frequency of filter. At this frequency, LP and HP modes each attenuate by 6 dB, and their outputs sum to 0 dB.", range: "20 - 20000 Hz", default: "2000" }
      - { name: Type, desc: "Filter mode. LP passes frequencies below cutoff. HP passes frequencies above. AP passes all frequencies but applies same phase shift as LP/HP modes.", range: "LP / HP / AP", default: "LP" }
---
::

**See also:** [$SN.filters.linkwitzriley$]($SN.filters.linkwitzriley$) -- deprecated alternative with stability issues when modulating frequency, [$SN.template.freq_split3$]($SN.template.freq_split3$) -- pre-built multiband splitter using jlinkwitzriley nodes
