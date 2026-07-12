---
title: math.square
description: "Squares each sample by multiplying it with itself."
factoryPath: math.square
factory: math
polyphonic: true
tags: [math, waveshaping]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso: []
commonMistakes: []
llmRef: |
  math.square

  Multiplies each sample by itself (s * s). Output is always non-negative. Doubles the frequency of the input and compresses dynamic range.

  Signal flow:
    audio in -> square -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 1.0): Not used. Present for internal reasons but has no effect on processing.

  When to use:
    Use as a simple waveshaper to produce non-negative output, double the frequency, or compress dynamic range. The Value parameter has no effect.
---

Multiplies each sample by itself. The output is always non-negative, which doubles the frequency of the input signal and compresses dynamic range. This acts as a simple waveshaper.

The Value parameter is present but has no effect on processing.

## Signal Path

::signal-path
---
glossary: {}
---

```
// math.square - squares the signal
// audio in -> audio out

process(input) {
    output = input * input
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for internal reasons but has no effect on the output.", range: "0.0 - 1.0", default: "1.0" }
---
::
