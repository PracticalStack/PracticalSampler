---
title: Abs
description: "Folds negative sample values to positive, producing the absolute value of the signal."
factoryPath: math.abs
factory: math
polyphonic: true
tags: [math, absolute-value, rectification]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.rect", type: disambiguation, reason: "Threshold-based binary gate vs continuous fold" }
commonMistakes: []
llmRef: |
  math.abs

  Computes the absolute value of each sample, folding negative values to positive.

  Signal flow:
    audio in -> abs(sample) -> audio out

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Full-wave rectification of an audio or modulation signal. Produces a non-negative output whilst preserving magnitude.

  See also:
    [disambiguation] math.rect -- threshold-based binary gate vs continuous fold
---

Computes the absolute value of each sample, folding negative values into the positive domain. This is the signal-processing equivalent of full-wave rectification.

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for interface consistency with other math nodes.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.math.rect$ -- threshold-based binary gate rather than continuous fold
