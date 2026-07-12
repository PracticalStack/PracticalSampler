---
title: SVF
description: "A state variable filter with five modes: lowpass, highpass, bandpass, notch, and allpass."
factoryPath: filters.svf
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/svf.png
polyphonic: true
tags: [filters, svf, lowpass, highpass, bandpass, notch, allpass]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.svf_eq", type: alternative, reason: "SVF with gain-dependent EQ modes (shelving, peak)" }
  - { id: "filters.biquad", type: alternative, reason: "Biquad with more mode variety but less modulation stability" }
  - { id: "PolyphonicFilter", type: module, reason: "Module-tree filter -- scriptnode offers individual filter types as separate nodes" }
commonMistakes:
  - title: "Gain parameter has no effect"
    wrong: "Setting the Gain parameter expecting it to affect the filter response"
    right: "Gain has no effect on any SVF mode. Use filters.svf_eq for gain-dependent filtering."
    explanation: "The Gain parameter exists for interface consistency across all filter nodes but is ignored by the SVF. For shelving or peak EQ, use filters.svf_eq instead."
  - title: "Keep Smoothing above zero for modulation"
    wrong: "Using filters.svf with rapid frequency modulation and Smoothing set to 0"
    right: "Keep Smoothing above 0 (default 0.01s) when modulating parameters to avoid zipper noise."
    explanation: "Although the SVF topology is inherently more stable than biquad under modulation, disabling smoothing entirely can still produce audible artefacts from discrete coefficient steps."
  - title: "Mode cannot be switched from the scriptnode graph"
    wrong: "Connecting a parameter to the Mode input expecting to switch filter type at runtime from scriptnode alone"
    right: "Control the Mode index from HiseScript using setAttribute on the node."
    explanation: "The Mode parameter is not exposed as a modulatable scriptnode parameter. To switch between LP, HP, BP, etc. at runtime, use a HiseScript callback that calls setAttribute with the desired mode index."
forumReferences:
  - { tid: 8941, summary: "Switching SVF mode at runtime requires HiseScript setAttribute" }
llmRef: |
  filters.svf

  State variable filter with five modes: LP, HP, BP, Notch, and Allpass. Second-order (12 dB/oct) with per-sample processing. Stable under modulation due to trapezoidal integration.

  Signal flow:
    audio in -> SVF (mode-dependent output selection) -> audio out

  CPU: low, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Cutoff or centre frequency.
    Q: 0.3 - 9.9 (default 1.0). Resonance. Higher values increase resonance but cannot self-oscillate.
    Gain: -18 - 18 dB (default 0). Ignored by all modes.
    Smoothing: 0 - 1 s (default 0.01). Parameter interpolation time.
    Mode: LP, HP, BP, Notch, Allpass (default LP). Not modulatable from scriptnode -- must be set via HiseScript setAttribute.
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    General-purpose filter for synthesis and effects. Preferred over biquad when modulating cutoff frequency. Use filters.svf_eq instead when shelving or peak EQ gain control is needed.

  Common mistakes:
    Gain parameter is ignored - use filters.svf_eq for gain-dependent modes.
    Mode cannot be switched from scriptnode graph connections -- use HiseScript setAttribute.

  See also:
    [alternative] filters.svf_eq - SVF with EQ/shelving modes
    [alternative] filters.biquad - biquad with more modes but less modulation stability
    [module] PolyphonicFilter - module-tree filter -- scriptnode offers individual filter types as separate nodes
---

A state variable filter with five modes: lowpass, highpass, bandpass, notch, and allpass. The SVF topology uses trapezoidal integration, which makes it stable under rapid frequency modulation - the recommended choice for filter sweeps in synthesiser patches and modulated effects.

![SVF screenshot](/images/v2/reference/scriptnodes/filters/svf.png)

Each voice maintains independent filter state. The Q parameter controls resonance but is scaled to prevent self-oscillation at maximum values.

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
      desc: "Resonance amount"
      range: "0.3 - 9.9"
      default: "1.0"
    Mode:
      desc: "Filter type: LP, HP, BP, Notch, Allpass"
      range: "0 - 4"
      default: "0 (LP)"
---

```
// filters.svf - state variable filter
// audio in -> audio out

process(input) {
    coefficients = computeSVF(Frequency, Q)

    if Mode == LP:
        output = svf.lowpass(input, coefficients)
    else if Mode == HP:
        output = svf.highpass(input, coefficients)
    else if Mode == BP:
        output = svf.bandpass(input, coefficients)
    else if Mode == Notch:
        output = svf.notch(input, coefficients)
    else if Mode == Allpass:
        output = svf.allpass(input, coefficients)
}
```

::

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - svf.LP.Q10.F1000
  - svf.HP.Q10.F1000
  - svf.BP.Q10.F1000
  - svf.Notch.Q10.F1000
  - svf.Allpass.Q10.F1000
---
::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Cutoff or centre frequency of the filter.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Resonance. Higher values produce a sharper peak at the cutoff frequency. Scaled internally to prevent self-oscillation.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Not used by any SVF mode. Present for interface consistency.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "Filter type. Cannot be modulated from the scriptnode graph. To switch modes at runtime, use HiseScript setAttribute.", range: "LP / HP / BP / Notch / Allpass", default: "LP" }
      - { name: Smoothing, desc: "Interpolation time for parameter changes. Prevents clicks when Frequency or Q are modulated.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass. When off, audio passes through unprocessed with no crossfade.", range: "Off / On", default: "On" }
---
::

### Allpass Mode

The Allpass mode uses a different internal formulation from the other four modes but shares the same parameter interface. It passes all frequencies at equal amplitude while shifting phase, useful for building phaser effects or correcting phase alignment.

### Bypass Behaviour

Toggling Enabled produces an instantaneous switch with no crossfade. For click-free bypass, use a [container.soft_bypass]($SN.container.soft_bypass$) wrapper instead.

**See also:** $SN.filters.svf_eq$ -- SVF with gain-dependent EQ modes, $SN.filters.biquad$ -- biquad with more mode variety but less modulation stability, $MODULES.PolyphonicFilter$ -- module-tree filter -- scriptnode offers individual filter types as separate nodes
