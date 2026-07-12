---
title: math.tanh
description: "Applies soft saturation using the hyperbolic tangent function."
factoryPath: math.tanh
factory: math
polyphonic: true
tags: [math, distortion, saturation]
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "math.clip", type: alternative, reason: "Hard clipping alternative" }
commonMistakes: []
llmRef: |
  math.tanh

  Applies hyperbolic tangent saturation to the signal. The Value parameter drives the input before the tanh function, controlling saturation intensity. Output is always bounded to (-1, 1).

  Signal flow:
    audio in -> multiply by Value -> tanh -> audio out

  CPU: medium, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 1.0): Drive amount. Higher values push the signal harder into saturation.

  When to use:
    Use for smooth saturation and soft-clipping effects. Produces warmer harmonics than hard clipping (math.clip). Higher CPU cost than simple arithmetic nodes.

  See also:
    alternative math.clip -- hard clipping alternative
---

Applies hyperbolic tangent saturation to the signal. The Value parameter scales the input before the tanh function is applied, controlling the amount of saturation. At 1.0 the effect is subtle; higher values drive the signal harder into the curve, producing more pronounced saturation. The output is always bounded to the range (-1, 1) regardless of input amplitude.

For hard clipping that truncates peaks abruptly, use [math.clip]($SN.math.clip$) instead.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Drive amount - scales input before saturation"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    tanh:
      desc: "Hyperbolic tangent function - soft-clips the signal to (-1, 1)"
---

```
// math.tanh - soft saturation
// audio in -> audio out

process(input) {
    output = tanh(input * Value)
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Drive amount. Scales the signal before the tanh function is applied. Higher values produce more saturation.", range: "0.0 - 1.0", default: "1.0" }
---
::

This node has a higher CPU cost than simple arithmetic math nodes because the tanh function is computed per sample without hardware acceleration. The output is always bounded to (-1, 1), making it inherently safe against clipping at the output stage.

**See also:** $SN.math.clip$ -- hard clipping alternative
