---
title: Rect
description: "Converts a normalised signal into a binary gate by thresholding at 0.5."
factoryPath: math.rect
factory: math
polyphonic: true
tags: [math, rectify, gate, threshold, binary]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.abs", type: disambiguation, reason: "Continuous absolute value vs binary threshold" }
commonMistakes: []
llmRef: |
  math.rect

  Converts a signal into a binary 0/1 gate using a fixed threshold of 0.5. Samples >= 0.5 become 1.0; samples below 0.5 become 0.0.

  Signal flow:
    mod in -> threshold at 0.5 -> binary out (0 or 1)

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Converting a continuous modulation signal into a square-wave gate. The threshold is fixed at 0.5.

  See also:
    [disambiguation] math.abs -- continuous absolute value vs binary threshold
---

Converts a signal into a binary 0 or 1 output using a fixed threshold of 0.5. Samples at or above 0.5 become 1.0; samples below 0.5 become 0.0. The threshold is hardcoded and cannot be changed.

This is useful for converting a continuous modulation signal into a square-wave gate. For bipolar audio signals, convert to the 0 to 1 range first using [math.sig2mod]($SN.math.sig2mod$) before applying rect.

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions: {}
---

```
// math.rect - binary threshold gate
// mod in -> binary out (0 or 1)

process(input) {
    if input >= 0.5:
        output = 1.0
    else:
        output = 0.0
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for interface consistency with other math nodes.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.math.abs$ -- continuous absolute value rather than binary threshold
