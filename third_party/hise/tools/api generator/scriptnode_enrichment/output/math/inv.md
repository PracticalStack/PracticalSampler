---
title: Inv
description: "Inverts the phase of a signal by negating every sample."
factoryPath: math.inv
factory: math
polyphonic: true
tags: [math, invert, negate, phase]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.mod_inv", type: disambiguation, reason: "Unipolar inversion (1 - x) for modulation signals" }
commonMistakes: []
llmRef: |
  math.inv

  Negates the signal by multiplying every sample by -1. This is bipolar phase inversion.

  Signal flow:
    audio in -> negate -> audio out

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Phase inversion of bipolar audio signals (flips the waveform around zero). For inverting a unipolar modulation signal (0-1), use math.mod_inv instead.

  See also:
    [disambiguation] math.mod_inv -- unipolar inversion (1 - x) for modulation signals
---

Negates every sample by multiplying it by -1, flipping the waveform around zero. This is bipolar phase inversion, suitable for audio signals in the -1 to 1 range.

For inverting a unipolar modulation signal (0 to 1 range), use [math.mod_inv]($SN.math.mod_inv$) instead, which computes 1 - x and keeps the output within the 0 to 1 range.

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for interface consistency with other math nodes.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.math.mod_inv$ -- unipolar inversion (1 - x) for modulation signals
