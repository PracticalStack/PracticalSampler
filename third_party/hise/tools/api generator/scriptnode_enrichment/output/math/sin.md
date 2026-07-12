---
title: Sin
description: "Applies the sine function to each sample of the signal."
factoryPath: math.sin
factory: math
polyphonic: true
tags: [math, sine, waveshaper, trigonometry]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.pi", type: companion, reason: "Prescales a ramp into radians before sin" }
commonMistakes: []
llmRef: |
  math.sin

  Applies the sine function to each sample. The input is interpreted as radians.

  Signal flow:
    audio in (radians) -> sin(sample) -> audio out

  CPU: medium, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 2.0). Not used in processing.

  When to use:
    Waveshaping a ramp into a sine wave. Typically preceded by math.pi (Value=2.0) to convert a 0-1 ramp into 0-2*PI radians, producing one complete sine cycle.

  See also:
    [companion] math.pi -- prescales a ramp into radians before sin
---

Applies the sine function to each sample. The input is interpreted as radians, so a value of 2*PI produces one complete cycle.

Typically preceded by [math.pi]($SN.math.pi$) with the default Value of 2.0, which converts a normalised 0 to 1 ramp into the 0 to 2*PI radian range needed for a full sine cycle. The sine computation is a transcendental function, making this node more expensive than simple arithmetic math nodes.

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions:
    sin:
      desc: "Trigonometric sine function (input in radians)"
---

```
// math.sin - sine waveshaper
// audio in (radians) -> audio out

process(input) {
    output = sin(input)
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for interface consistency with other math nodes.", range: "0.0 - 1.0", default: "2.0" }
---
::

**See also:** $SN.math.pi$ -- prescales a normalised ramp into radians before sin
