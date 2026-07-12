---
title: Pow
description: "Raises each sample to a fixed power, shaping the signal curve."
factoryPath: math.pow
factory: math
polyphonic: true
tags: [math, power, curve, waveshaper]
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.sqrt", type: alternative, reason: "Square root (equivalent to pow with exponent 0.5)" }
commonMistakes:
  - title: "Negative input produces NaN"
    wrong: "Feeding a bipolar audio signal directly into math.pow"
    right: "Place math.abs before math.pow, or ensure the input is in the 0-1 range"
    explanation: "Raising a negative number to a fractional power is undefined. Negative input samples will produce NaN, which can corrupt the entire signal chain."
llmRef: |
  math.pow

  Raises each sample to a power. Intended for non-negative (unipolar) signal curve shaping.

  Signal flow:
    mod in (0-1) -> pow(sample) -> mod out

  CPU: medium, polyphonic

  Parameters:
    Value: 0.0 - 1.0 (default 1.0). Not used in processing.

  When to use:
    Shaping modulation curves. Ensure input is non-negative.

  Common mistakes:
    Negative input produces NaN with fractional exponents.

  See also:
    [alternative] math.sqrt -- square root (equivalent to pow with exponent 0.5)
---

Raises each sample to a power, shaping the signal curve. This is intended for unipolar signals in the 0 to 1 range.

Like [math.sqrt]($SN.math.sqrt$), this node expects non-negative input. Negative sample values with fractional exponents will produce NaN, which can corrupt the downstream signal chain.

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions:
    pow:
      desc: "Power function (raises sample to an exponent)"
---

```
// math.pow - power function waveshaper
// mod in (0-1) -> mod out

process(input) {
    output = pow(input)
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

**See also:** $SN.math.sqrt$ -- square root (equivalent to pow with exponent 0.5)
