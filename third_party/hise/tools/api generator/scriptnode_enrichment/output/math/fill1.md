---
title: Fill 1
description: "Replaces the signal with a constant DC value of 1.0."
factoryPath: math.fill1
factory: math
polyphonic: true
tags: [math, dc, constant, fill]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes: []
llmRef: |
  math.fill1

  Replaces the entire signal with a constant value of 1.0. The input is discarded.

  Signal flow:
    audio in -> 1.0 -> audio out

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Creating a DC reference signal for modulation arithmetic or testing. Place before math.mul or math.add to build constant offsets.
---

Replaces the entire signal with a constant value of 1.0 on every sample. The input is completely discarded.

Useful for generating a DC reference signal that can then be shaped by subsequent math nodes, for example multiplying by a parameter value to produce an arbitrary constant.

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for interface consistency with other math nodes.", range: "0.0 - 1.0", default: "0.0" }
---
::
