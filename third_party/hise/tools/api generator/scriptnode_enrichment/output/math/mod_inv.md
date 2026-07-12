---
title: Mod Inv
description: "Inverts a unipolar modulation signal within the 0-1 range."
factoryPath: math.mod_inv
factory: math
polyphonic: true
tags: [math, invert, modulation, unipolar]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.inv", type: disambiguation, reason: "Bipolar phase inversion (negation) for audio signals" }
commonMistakes: []
llmRef: |
  math.mod_inv

  Inverts a unipolar signal by computing 1 - x. An input of 0 becomes 1 and vice versa.

  Signal flow:
    mod in -> (1 - sample) -> mod out

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Inverting a modulation signal so that maximum becomes minimum. For bipolar audio inversion (negation), use math.inv instead.

  See also:
    [disambiguation] math.inv -- bipolar phase inversion (negation) for audio signals
---

Inverts a unipolar modulation signal by computing 1 - x. An input of 0.0 becomes 1.0 and an input of 1.0 becomes 0.0, mirroring the signal within the 0 to 1 range.

This is distinct from [math.inv]($SN.math.inv$), which negates the signal (multiplies by -1). Use mod_inv for modulation signals in the 0 to 1 range; use inv for bipolar audio signals.

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions: {}
---

```
// math.mod_inv - unipolar inversion
// mod in -> mod out

process(input) {
    output = 1.0 - input
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

**See also:** $SN.math.inv$ -- bipolar phase inversion (negation) for audio signals
