---
title: math.intensity
description: "Controls modulation depth by crossfading between unity and the input signal."
factoryPath: math.intensity
factory: math
polyphonic: true
tags: [math, modulation, intensity]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.mul", type: disambiguation, reason: "Plain multiplication vs intensity crossfade" }
commonMistakes: []
llmRef: |
  math.intensity

  Crossfades between 1.0 and the input signal using the formula: output = (1 - Value) + Value * input. Controls the depth of a modulation signal.

  Signal flow:
    audio in -> intensity crossfade -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): Intensity depth. 0.0 = output is always 1.0 (no effect), 1.0 = output equals the input (full depth).

  When to use:
    Use on a modulation signal to scale its depth while keeping the ceiling at 1.0. Unlike math.mul, which scales around zero, intensity scales around 1.0 - so a 0-to-1 modulation signal at Value 0.5 becomes 0.5-to-1 rather than 0-to-0.5.

  See also:
    disambiguation math.mul -- plain multiplication (scales around zero)
---

Crossfades between a constant value of 1.0 and the input signal. The formula is `output = (1 - Value) + Value * input`. At a Value of 0.0 the output is always 1.0 regardless of the input; at 1.0 the output equals the input unchanged.

This is designed for controlling modulation depth. Unlike [math.mul]($SN.math.mul$), which scales around zero, intensity scales around 1.0. When applied to a 0-to-1 modulation signal with Value set to 0.5, the output range becomes 0.5-to-1 rather than 0-to-0.5, halving the modulation depth while keeping the ceiling at 1.0.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Intensity depth - crossfade between 1.0 and the input"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// math.intensity - modulation depth control
// audio in -> audio out

process(input) {
    output = (1 - Value) + Value * input
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Intensity depth. At 0.0 the output is always 1.0. At 1.0 the output equals the input. Intermediate values crossfade between unity and the input signal.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.math.mul$ -- plain multiplication (scales around zero)
