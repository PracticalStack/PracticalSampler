---
title: math.mul
description: "Multiplies the signal by a scalar value for gain staging and level control."
factoryPath: math.mul
factory: math
polyphonic: true
tags: [math, arithmetic, gain]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.div", type: alternative, reason: "Division counterpart" }
  - { id: "core.gain", type: companion, reason: "Decibel-scaled gain with smoothing" }
commonMistakes: []
llmRef: |
  math.mul

  Multiplies the input signal by a scalar Value parameter. The most common math node, used for gain staging and level control.

  Signal flow:
    audio in -> multiply by Value -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 1.0): Scalar multiplier. 1.0 = passthrough, 0.0 = silence.

  When to use:
    Use for raw gain scaling. For decibel-based volume control, pair with a control.converter node set to db2Gain mode, or use core.gain instead.

  See also:
    alternative math.div -- division counterpart
    companion core.gain -- decibel-scaled gain with smoothing
---

Multiplies every sample in the signal by the Value parameter. At 1.0 the signal passes through unchanged; at 0.0 the output is silence. This is the most direct way to scale a signal's amplitude inside a scriptnode network.

For decibel-based volume control, pair this node with a [control.converter]($SN.control.converter$) set to `db2Gain` mode, or use [core.gain]($SN.core.gain$) which provides decibel scaling and parameter smoothing in a single node.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Scalar multiplier applied to each sample"
      range: "0.0 - 1.0"
      default: "1.0"
---

```
// math.mul - multiplies the signal by a scalar
// audio in -> audio out

process(input) {
    output = input * Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Scalar multiplier. 1.0 passes the signal through unchanged, 0.0 produces silence.", range: "0.0 - 1.0", default: "1.0" }
---
::

**See also:** $SN.math.div$ -- division counterpart, $SN.core.gain$ -- decibel-scaled gain with smoothing
