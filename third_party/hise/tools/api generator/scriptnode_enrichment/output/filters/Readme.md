---
title: Filter Nodes
description: "The filters factory contains nodes for spectral shaping, frequency-band splitting, and modulation effects."
factory: filters
---

The filters factory contains nodes for spectral shaping, frequency-band splitting, and modulation effects. Most filter nodes share a common parameter interface (Frequency, Q, Gain, Smoothing, Mode, Enabled), though not all parameters are active for every node - check the individual node pages for which parameters are used.

All filter nodes except [filters.convolution]($SN.filters.convolution$) support polyphonic operation with independent state per voice. Parameter changes are smoothed by default (Smoothing = 0.01s) to prevent clicks.

## Frequency Response

::filter-response
---
dataUrl: /data/filter-curves.json
defaultResponse: magnitude
defaults:
  - svf.LP.Q10.F1000
  - biquad.LP.F1000
  - one_pole.LP.F1000
---
::

## Nodes

| Node | Description |
|------|-------------|
| [$SN.filters.svf$]($SN.filters.svf$) | State variable filter with LP, HP, BP, Notch, and Allpass modes. Stable under modulation. |
| [$SN.filters.svf_eq$]($SN.filters.svf_eq$) | SVF parametric EQ with LowPass, HighPass, LowShelf, HighShelf, and Peak modes. Responds to Gain. |
| [$SN.filters.biquad$]($SN.filters.biquad$) | Second-order biquad with six modes including ResoLow. Widest mode selection. |
| [$SN.filters.one_pole$]($SN.filters.one_pole$) | First-order filter (6 dB/oct). LP and HP modes. Minimal CPU. |
| [$SN.filters.moog$]($SN.filters.moog$) | Moog-style transistor ladder lowpass (24 dB/oct) with analog character. |
| [$SN.filters.ladder$]($SN.filters.ladder$) | Simple 4-pole ladder lowpass (24 dB/oct). Lightest multi-pole filter. |
| [$SN.filters.linkwitzriley$]($SN.filters.linkwitzriley$) | LR4 crossover filter (24 dB/oct) with LP, HP, and Allpass outputs. |
| [$SN.filters.allpass$]($SN.filters.allpass$) | Six-stage cascaded allpass chain with feedback (phaser effect). |
| [$SN.filters.convolution$]($SN.filters.convolution$) | FFT convolution reverb with impulse response loading. Monophonic, high CPU. |
| [$SN.filters.ring_mod$]($SN.filters.ring_mod$) | Ring modulator with internal sine oscillator. |
