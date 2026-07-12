---
title: Sig2Mod
description: "Converts a bipolar audio signal (-1 to 1) to a unipolar modulation signal (0-1)."
factoryPath: math.sig2mod
factory: math
polyphonic: true
tags: [math, conversion, modulation, bipolar, unipolar]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.mod2sig", type: companion, reason: "Inverse operation (unipolar to bipolar)" }
commonMistakes: []
llmRef: |
  math.sig2mod

  Converts a bipolar signal (-1 to 1) to a unipolar signal (0-1) using the formula: output = input * 0.5 + 0.5.

  Signal flow:
    audio in (-1 to 1) -> scale and offset -> mod out (0-1)

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Converting an audio-range signal (e.g. from an oscillator) into the normalised modulation range that peak nodes and other modulation targets expect.

  See also:
    [companion] math.mod2sig -- inverse operation (unipolar to bipolar)
---

Converts a bipolar signal in the -1 to 1 range to a unipolar modulation signal in the 0 to 1 range. The primary use case is converting an audio signal (for example from an oscillator) into the normalised range that modulation peak nodes expect.

This is the inverse of [math.mod2sig]($SN.math.mod2sig$).

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions: {}
---

```
// math.sig2mod - bipolar to unipolar conversion
// audio in (-1 to 1) -> mod out (0-1)

process(input) {
    output = input * 0.5 + 0.5
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

**See also:** $SN.math.mod2sig$ -- inverse operation (unipolar to bipolar)
