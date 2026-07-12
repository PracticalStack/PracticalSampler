---
title: Mod2Sig
description: "Converts a unipolar modulation signal (0-1) to a bipolar audio signal (-1 to 1)."
factoryPath: math.mod2sig
factory: math
polyphonic: true
tags: [math, conversion, modulation, bipolar, unipolar]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.sig2mod", type: companion, reason: "Inverse operation (bipolar to unipolar)" }
commonMistakes: []
llmRef: |
  math.mod2sig

  Converts a unipolar signal (0-1) to a bipolar signal (-1 to 1) using the formula: output = input * 2 - 1.

  Signal flow:
    mod in (0-1) -> scale and offset -> audio out (-1 to 1)

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Converting a modulation-range signal to audio range. Inverse of math.sig2mod.

  See also:
    [companion] math.sig2mod -- inverse operation (bipolar to unipolar)
---

Converts a unipolar modulation signal in the 0 to 1 range to a bipolar signal in the -1 to 1 range. This is the inverse of [math.sig2mod]($SN.math.sig2mod$).

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions: {}
---

```
// math.mod2sig - unipolar to bipolar conversion
// mod in (0-1) -> audio out (-1 to 1)

process(input) {
    output = input * 2.0 - 1.0
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

**See also:** $SN.math.sig2mod$ -- inverse operation (bipolar to unipolar)
