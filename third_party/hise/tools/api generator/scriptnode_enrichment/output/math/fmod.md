---
title: math.fmod
description: "Computes the floating-point remainder of each sample divided by the Value parameter."
factoryPath: math.fmod
factory: math
polyphonic: true
tags: [math, modulo, waveshaping]
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors: []
seeAlso: []
commonMistakes: []
llmRef: |
  math.fmod

  Computes the floating-point remainder (modulo) of each sample divided by the Value parameter. Wraps the signal into a repeating range.

  Signal flow:
    audio in -> fmod(sample, Value) -> audio out

  CPU: medium, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 1.0): Modulo divisor. At 0.0 the signal passes through unchanged (zero guard).

  When to use:
    Use for waveshaping effects that wrap a signal into a bounded range, producing sharp harmonics. Higher CPU cost than simple arithmetic nodes due to the per-sample modulo calculation.
---

Computes the floating-point remainder of each sample divided by the Value parameter. This wraps the signal into a repeating range, which can be used for waveshaping effects that produce sharp harmonic content.

At a Value of 0.0 the signal passes through unchanged as a safety guard against division by zero.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Modulo divisor"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    fmod:
      desc: "Floating-point remainder after dividing the sample by Value"
---

```
// math.fmod - floating-point modulo
// audio in -> audio out

process(input) {
    output = fmod(input, Value)
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Modulo divisor. Each sample is wrapped to the remainder after division by this value. At 0.0 the signal passes through unchanged.", range: "0.0 - 1.0", default: "1.0" }
---
::

This node has a higher CPU cost than simple arithmetic math nodes because the modulo operation is computed per sample without hardware acceleration.
