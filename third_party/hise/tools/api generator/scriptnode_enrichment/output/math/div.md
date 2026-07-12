---
title: math.div
description: "Divides the signal by a scalar value."
factoryPath: math.div
factory: math
polyphonic: true
tags: [math, arithmetic]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.mul", type: alternative, reason: "Multiplication counterpart" }
  - { id: "core.gain", type: companion, reason: "Decibel-scaled gain with smoothing" }
commonMistakes:
  - title: "Zero or negative Value produces silence"
    wrong: "Setting Value to 0.0 or a negative number expecting division"
    right: "Use only positive values for the divisor"
    explanation: "Values of zero or below produce silence rather than division by zero or negative division. Only positive Value settings perform actual division."
llmRef: |
  math.div

  Divides every sample in the signal by the Value parameter. Only positive values produce division; zero and negative values produce silence.

  Signal flow:
    audio in -> divide by Value -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 1.0): Divisor. Only positive values perform division. Zero and negative values produce silence.

  When to use:
    Use to attenuate a signal by a known factor. For most gain-staging tasks, math.mul is more intuitive.

  Common mistakes:
    Zero or negative values produce silence rather than an error.

  See also:
    alternative math.mul -- multiplication counterpart
    companion core.gain -- decibel-scaled gain with smoothing
---

Divides every sample in the signal by the Value parameter. At 1.0 the signal passes through unchanged. Only positive values perform division; zero and negative values produce silence as a safety guard.

For most gain-staging tasks, [math.mul]($SN.math.mul$) is more intuitive since you can directly set the desired gain factor. This node cannot be used for phase inversion via negative divisors.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Divisor applied to each sample (must be positive)"
      range: "0.0 - 1.0"
      default: "1.0"
---

```
// math.div - divides the signal by a scalar
// audio in -> audio out

process(input) {
    output = input / Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Divisor. Only positive values perform division. Zero and negative values produce silence.", range: "0.0 - 1.0", default: "1.0" }
---
::

### Limitations

Zero and negative Value settings produce silence rather than causing errors. This means the node cannot perform negative division or phase inversion.

**See also:** $SN.math.mul$ -- multiplication counterpart, $SN.core.gain$ -- decibel-scaled gain with smoothing
