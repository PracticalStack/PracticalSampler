---
title: Pi
description: "Multiplies the signal by PI times the Value parameter."
factoryPath: math.pi
factory: math
polyphonic: true
tags: [math, pi, multiply, radians]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.sin", type: companion, reason: "Typically follows pi to convert a ramp into a sine wave" }
commonMistakes: []
llmRef: |
  math.pi

  Multiplies the signal by PI * Value. At the default Value of 2.0, this multiplies by 2*PI (a full radian cycle).

  Signal flow:
    audio in -> multiply by (PI * Value) -> audio out

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 2.0). Multiplier applied together with PI. At 2.0, the combined factor is 2*PI.

  When to use:
    Prescaling a normalised ramp (0-1) into radians before feeding it to math.sin. With Value=2.0 (default), a 0-1 ramp becomes 0-2*PI, producing one complete sine cycle.

  See also:
    [companion] math.sin -- typically follows pi to apply the sine function
---

Multiplies the signal by PI times the Value parameter. With the default Value of 2.0, this multiplies by 2*PI (approximately 6.283), which converts a normalised 0 to 1 ramp into a full radian cycle suitable for [math.sin]($SN.math.sin$).

This saves you from manually typing PI with the correct precision. The combined formula is `output = input * PI * Value`.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Multiplier applied together with PI"
      range: "0.0 - 1.0"
      default: "2.0"
---

```
// math.pi - prescale by PI
// audio in -> audio out

process(input) {
    output = input * PI * Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Multiplier applied together with PI. At the default of 2.0, the combined factor is 2*PI (one full radian cycle).", range: "0.0 - 1.0", default: "2.0" }
---
::

**See also:** $SN.math.sin$ -- typically follows pi to apply the sine function
