---
title: math.sub
description: "Subtracts a constant value from the signal."
factoryPath: math.sub
factory: math
polyphonic: true
tags: [math, arithmetic]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.add", type: alternative, reason: "Addition counterpart" }
commonMistakes: []
llmRef: |
  math.sub

  Subtracts a constant scalar value from every sample in the signal.

  Signal flow:
    audio in -> subtract Value -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): Constant subtracted from each sample. 0.0 = passthrough.

  When to use:
    Use to remove a known DC offset or shift a signal downwards. The addition counterpart is math.add.

  See also:
    alternative math.add -- addition counterpart
---

Subtracts the Value parameter from every sample in the signal. At 0.0 the signal passes through unchanged. This is the inverse of [math.add]($SN.math.add$) and can be used to remove a known DC offset.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Constant subtracted from each sample"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// math.sub - subtracts a constant
// audio in -> audio out

process(input) {
    output = input - Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Constant value subtracted from each sample. 0.0 passes the signal through unchanged.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.math.add$ -- addition counterpart
