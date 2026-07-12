---
title: Clear
description: "Sets every sample to zero, silencing the signal."
factoryPath: math.clear
factory: math
polyphonic: true
tags: [math, silence, clear, signal-reset]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes: []
llmRef: |
  math.clear

  Replaces the entire signal with silence (zero). The input is discarded.

  Signal flow:
    audio in -> 0.0 -> audio out

  CPU: negligible, monophonic processing

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Not used in processing.

  When to use:
    Terminate a signal path so that upstream audio does not leak into downstream processing. Commonly used at the end of a control-signal chain to prevent the control signal from reaching the audio output.
---

Replaces the entire signal with silence by setting every sample to zero. The input is completely discarded.

This is particularly useful when building control-signal chains that must not leak into the audio output. Place a clear node at the end of the chain to ensure only the control values (via modulation connections) pass through, not the raw signal. During development, a clear node at the end of a chain is also handy as a mute that can be bypassed when you want to hear the output.

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Not used. Present for interface consistency with other math nodes.", range: "0.0 - 1.0", default: "0.0" }
---
::
