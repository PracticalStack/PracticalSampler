---
title: Biquad
description: "A second-order biquad filter with six modes: LowPass, HighPass, LowShelf, HighShelf, Peak, and ResoLow."
factoryPath: filters.biquad
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/biquad.png
polyphonic: true
tags: [filters, biquad, iir, lowpass, highpass, lowshelf, highshelf, peak, eq]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.svf", type: alternative, reason: "SVF with better modulation stability" }
  - { id: "filters.svf_eq", type: alternative, reason: "SVF EQ with gain modes and better modulation stability" }
  - { id: "PolyphonicFilter", type: module, reason: "Module-tree filter -- scriptnode offers individual filter types as separate nodes" }
commonMistakes:
  - title: "Unstable under fast modulation"
    wrong: "Using filters.biquad with fast frequency modulation from an LFO or envelope"
    right: "Use filters.svf or filters.svf_eq for modulated filter sweeps."
    explanation: "The biquad topology can produce clicks or instability when coefficients change rapidly. SVF filters handle modulation more gracefully."
  - title: "Use ResoLow for resonant lowpass"
    wrong: "Expecting resonance in LowPass mode (mode 0)"
    right: "Use ResoLow mode (mode 5) for a resonant lowpass."
    explanation: "LowPass mode uses a Butterworth design with no resonance control. Q is ignored. Switch to ResoLow for a resonant lowpass that responds to Q."
llmRef: |
  filters.biquad

  Second-order biquad (IIR) filter with six modes: LowPass, HighPass, LowShelf, HighShelf, Peak, and ResoLow. Uses standard cookbook formulas.

  Signal flow:
    audio in -> biquad filter -> audio out

  CPU: low, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Cutoff or centre frequency.
    Q: 0.3 - 9.9 (default 1.0). Used by LowShelf, HighShelf, Peak, ResoLow. Ignored by LowPass, HighPass.
    Gain: -18 - 18 dB (default 0). Used by LowShelf, HighShelf, Peak. Ignored by LowPass, HighPass, ResoLow.
    Smoothing: 0 - 1 s (default 0.01). Coefficient interpolation time.
    Mode: LowPass, HighPass, LowShelf, HighShelf, Peak, ResoLow (default LowPass).
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    Static or slowly changing EQ. Has the widest mode selection of any filter node (6 modes). Avoid for fast modulation - use SVF filters instead.

  Common mistakes:
    LowPass mode has no resonance. Use ResoLow for resonant lowpass.
    Susceptible to clicks with fast modulation. Use SVF filters for filter sweeps.

  See also:
    [alternative] filters.svf - SVF with better modulation stability
    [alternative] filters.svf_eq - SVF EQ with gain modes
    [module] PolyphonicFilter - module-tree filter -- scriptnode offers individual filter types as separate nodes
---

A second-order biquad filter with six modes: LowPass, HighPass, LowShelf, HighShelf, Peak, and ResoLow. This provides the widest mode selection of any filter node, including a dedicated resonant lowpass mode separate from the standard Butterworth lowpass.

![Biquad screenshot](/images/v2/reference/scriptnodes/filters/biquad.png)

The biquad uses a standard Direct Form II topology. It is well suited for static EQ and slowly changing parameters but can produce clicks or instability under rapid modulation. For filter sweeps driven by envelopes or LFOs, prefer [filters.svf]($SN.filters.svf$) or [filters.svf_eq]($SN.filters.svf_eq$).

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
      desc: "Resonance or bandwidth (mode-dependent)"
      range: "0.3 - 9.9"
      default: "1.0"
    Gain:
      desc: "Boost/cut for shelf and peak modes"
      range: "-18 - 18 dB"
      default: "0.0"
    Mode:
      desc: "Filter type"
      range: "LowPass / HighPass / LowShelf / HighShelf / Peak / ResoLow"
      default: "LowPass"
---

```
// filters.biquad - second-order IIR filter
// audio in -> audio out

process(input) {
    coefficients = computeBiquad(Mode, Frequency, Q, Gain)
    output = biquad.process(input, coefficients)
}
```

::

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - biquad.LP.F1000
  - biquad.HP.F1000
  - biquad.ResoLow.Q10.F1000
  - biquad.LowShelf.Q10.F1000
  - biquad.HighShelf.Q10.F1000
  - biquad.Peak.Q10.F1000
---
::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Cutoff or centre frequency of the filter.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Resonance (ResoLow), bandwidth (Peak), or slope (shelves). Ignored by LowPass and HighPass modes.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Boost or cut in decibels. Only affects LowShelf, HighShelf, and Peak modes.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "Filter type. LowPass and HighPass are Butterworth (no resonance). ResoLow is a resonant lowpass.", range: "LowPass / HighPass / LowShelf / HighShelf / Peak / ResoLow", default: "LowPass" }
      - { name: Smoothing, desc: "Interpolation time for parameter changes. Keep above zero to prevent clicks.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass.", range: "Off / On", default: "On" }
---
::

### Limitations

Changing the Mode parameter at runtime resets the filter state, which can produce a click. Avoid switching modes during audio playback.

**See also:** $SN.filters.svf$ -- SVF with better modulation stability, $SN.filters.svf_eq$ -- SVF EQ with gain modes and better modulation stability, $MODULES.PolyphonicFilter$ -- module-tree filter -- scriptnode offers individual filter types as separate nodes
