---
title: SVF EQ
description: "An SVF-based parametric EQ with five modes: LowPass, HighPass, LowShelf, HighShelf, and Peak."
factoryPath: filters.svf_eq
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/svf_eq.png
polyphonic: true
tags: [filters, svf, eq, lowshelf, highshelf, peak, parametric]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.svf", type: alternative, reason: "SVF without gain modes (LP/HP/BP/Notch/Allpass)" }
  - { id: "filters.biquad", type: alternative, reason: "Biquad with similar EQ modes but different topology" }
  - { id: "CurveEq", type: module, reason: "Scriptnode SVF-based parametric EQ" }
commonMistakes:
  - title: "Gain only works in shelf and peak modes"
    wrong: "Setting Gain and expecting it to affect LowPass or HighPass modes"
    right: "Gain only affects LowShelf, HighShelf, and Peak modes."
    explanation: "In LowPass and HighPass modes the Gain parameter is ignored. Switch to a shelf or peak mode to control boost/cut."
llmRef: |
  filters.svf_eq

  SVF-based parametric EQ with five modes: LowPass, HighPass, LowShelf, HighShelf, and Peak. Uses double-precision processing with built-in per-sample coefficient smoothing for zipper-free parameter changes.

  Signal flow:
    audio in -> SVF EQ (mixing-matrix output selection) -> audio out

  CPU: low, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Cutoff or centre frequency.
    Q: 0.3 - 9.9 (default 1.0). Bandwidth or resonance.
    Gain: -18 - 18 dB (default 0). Boost/cut for LowShelf, HighShelf, Peak. Ignored by LowPass, HighPass.
    Smoothing: 0 - 1 s (default 0.01). Parameter interpolation time.
    Mode: LowPass, HighPass, LowShelf, HighShelf, Peak (default LowPass).
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    Parametric EQ in scriptnode. Preferred over filters.biquad for modulated EQ due to SVF stability. Use filters.svf instead when gain control is not needed.

  Common mistakes:
    Gain is ignored in LowPass/HighPass modes - use LowShelf/HighShelf/Peak for boost/cut.

  See also:
    [alternative] filters.svf - SVF without gain modes
    [alternative] filters.biquad - biquad with similar EQ modes
    [module] CurveEq - scriptnode SVF-based parametric EQ
---

An SVF-based parametric EQ with five modes: LowPass, HighPass, LowShelf, HighShelf, and Peak. Unlike [filters.svf]($SN.filters.svf$), this node responds to the Gain parameter in its shelving and peak modes, making it the primary choice for EQ bands in scriptnode.

![SVF EQ screenshot](/images/v2/reference/scriptnodes/filters/svf_eq.png)

The filter uses double-precision arithmetic and includes built-in per-sample coefficient smoothing on top of the standard Smoothing parameter, providing particularly clean parameter transitions. This is the most-used filter node in practice.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Cutoff or centre frequency"
      range: "20 - 20000 Hz"
      default: "1000"
    Q:
      desc: "Bandwidth or resonance"
      range: "0.3 - 9.9"
      default: "1.0"
    Gain:
      desc: "Boost/cut amount for shelf and peak modes"
      range: "-18 - 18 dB"
      default: "0.0"
    Mode:
      desc: "EQ band type"
      range: "LowPass / HighPass / LowShelf / HighShelf / Peak"
      default: "LowPass"
---

```
// filters.svf_eq - SVF parametric EQ
// audio in -> audio out

process(input) {
    mixCoeffs = computeMixMatrix(Mode, Frequency, Q, Gain)

    // unified SVF with mixing matrix
    v0, v1, v2 = svf.process(input, mixCoeffs)
    output = mixCoeffs.m0 * v0 + mixCoeffs.m1 * v1 + mixCoeffs.m2 * v2
}
```

::

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - svf_eq.LP.Q10.F1000
  - svf_eq.HP.Q10.F1000
  - svf_eq.Peak.Q10.F1000
  - svf_eq.LowShelf.Q10.F1000
  - svf_eq.HighShelf.Q10.F1000
---
::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Cutoff or centre frequency of the filter.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Bandwidth (Peak) or resonance (LowPass/HighPass). Higher values produce a narrower bell in Peak mode.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Boost or cut in decibels. Only affects LowShelf, HighShelf, and Peak modes. Ignored by LowPass and HighPass.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "EQ band type.", range: "LowPass / HighPass / LowShelf / HighShelf / Peak", default: "LowPass" }
      - { name: Smoothing, desc: "Interpolation time for parameter changes.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass. When off, audio passes through unprocessed.", range: "Off / On", default: "On" }
---
::

### Precision

The double-precision processing provides slightly better numerical accuracy than [filters.svf]($SN.filters.svf$), which can matter when cascading multiple EQ bands.

**See also:** $SN.filters.svf$ -- SVF without gain modes (LP/HP/BP/Notch/Allpass), $SN.filters.biquad$ -- biquad with similar EQ modes but different topology, $MODULES.CurveEq$ -- scriptnode SVF-based parametric EQ
