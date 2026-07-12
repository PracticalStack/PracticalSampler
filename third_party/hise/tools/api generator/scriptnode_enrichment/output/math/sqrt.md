---
title: Sqrt
description: "Applies the square root function to each sample of the signal."
factoryPath: math.sqrt
factory: math
polyphonic: true
tags: [math, square-root, curve, waveshaper]
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.pow", type: alternative, reason: "Generalised power function for arbitrary curve shaping" }
commonMistakes:
  - title: "Negative input produces NaN"
    wrong: "Feeding a bipolar audio signal directly into math.sqrt"
    right: "Place math.abs before math.sqrt, or ensure the input is in the 0-1 range"
    explanation: "The square root of a negative number is undefined. Negative input samples will produce NaN, which can corrupt the entire signal chain."
llmRef: |
  math.sqrt

  Applies the square root function to each sample. Intended for non-negative (unipolar) signals.

  Signal flow:
    mod in (0-1) -> sqrt(sample) -> mod out

  CPU: medium, polyphonic

  Parameters:
    Value: 0.0 - 1.0 (default 1.0). Not used in processing.

  When to use:
    Shaping a modulation curve to be more concave (rises quickly then flattens). Ensure input is non-negative to avoid NaN.

  Common mistakes:
    Negative input produces NaN. Place math.abs before sqrt if the input may contain negative values.

  See also:
    [alternative] math.pow -- generalised power function for arbitrary curve shaping
---

Applies the square root function to each sample. This produces a concave curve on unipolar signals: values rise quickly near zero and flatten towards 1.0, which is useful for shaping modulation response curves.

This node expects non-negative input. Negative sample values will produce NaN, which can corrupt the downstream signal chain. If the input may contain negative values, place [math.abs]($SN.math.abs$) before this node.

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions:
    sqrt:
      desc: "Square root function (input must be non-negative)"
---

```
// math.sqrt - square root waveshaper
// mod in (0-1) -> mod out

process(input) {
    output = sqrt(input)
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for interface consistency with other math nodes.", range: "0.0 - 1.0", default: "1.0" }
---
::

**See also:** $SN.math.pow$ -- generalised power function for arbitrary curve shaping
