---
title: math.add
description: "Adds a constant DC offset to the signal."
factoryPath: math.add
factory: math
polyphonic: true
tags: [math, arithmetic, dc-offset]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.sub", type: alternative, reason: "Subtraction counterpart" }
commonMistakes: []
llmRef: |
  math.add

  Adds a constant scalar value (DC offset) to every sample in the signal.

  Signal flow:
    audio in -> add Value -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): Constant added to each sample. 0.0 = passthrough.

  When to use:
    Use to introduce a DC offset - for example, shifting a bipolar signal before asymmetric distortion, or adding 1.0 to build a modulation signal. Not suitable for volume changes (use math.mul or core.gain instead).

  See also:
    alternative math.sub -- subtraction counterpart
---

Adds the Value parameter to every sample in the signal, applying a constant DC offset. At 0.0 the signal passes through unchanged.

Common uses include shifting a bipolar signal before applying asymmetric distortion (followed by a high-pass filter to remove the offset afterwards), and adding 1.0 to a signal as part of building a modulation source.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Constant added to each sample (DC offset)"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// math.add - adds a DC offset
// audio in -> audio out

process(input) {
    output = input + Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Constant value added to each sample. 0.0 passes the signal through unchanged.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.math.sub$ -- subtraction counterpart
