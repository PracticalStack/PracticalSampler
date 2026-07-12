---
title: Random
description: "Generates a random value between 0.0 and 1.0 each time the Value parameter changes."
factoryPath: control.random
factory: control
polyphonic: false
tags: [control, random, modulation]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.midi", type: companion, reason: "The 'random' mode on control.midi generates a random value per note-on" }
commonMistakes:
  - title: "Input value is ignored"
    wrong: "Sending specific values to the Value parameter expecting them to influence the random output range"
    right: "The input value is completely ignored. Any change to the Value parameter triggers a new random output regardless of what value was sent."
    explanation: "The Value parameter acts purely as a trigger. The random output is always uniformly distributed between 0.0 and 1.0."
llmRef: |
  control.random

  Generates a random value (0..1, uniform distribution) each time the Value parameter changes. The input value is ignored -- any change triggers a new random output.

  Signal flow:
    Control node -- no audio processing
    Value change (any) -> generate random 0..1 -> normalised modulation output

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Trigger input. The value itself is ignored; any change generates a new random output.

  When to use:
    Adding randomisation to modulation chains. 3 instances in surveyed projects (rank 68). Connect a trigger source (e.g. control.bang, control.midi) to the Value input to generate a new random value on each trigger.

  See also:
    [companion] control.midi -- random mode generates per-note random values
---

Generates a new random value between 0.0 and 1.0 (uniform distribution) every time the Value parameter receives any change. The actual value sent to the input is completely ignored -- it serves purely as a trigger. Each change produces a fresh random number at the output.

The output is normalised, so the connection system applies the target parameter's range to map the 0..1 value appropriately.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Trigger input -- any change generates a new random value (input value ignored)"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    generateRandom:
      desc: "Produces a uniformly distributed random value between 0.0 and 1.0"
---

```
// control.random - random value generator
// trigger in -> random value out (0..1)

onValueChange(input) {
    // input value is ignored
    output = generateRandom()   // uniform 0.0 to 1.0
    sendToOutput(output)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Trigger input. The value itself is ignored -- any change generates a new random output between 0.0 and 1.0.", range: "0.0 - 1.0", default: "0.0" }
---
::

Connect a trigger source such as [control.bang]($SN.control.bang$) or a gate signal to the Value input to control when new random values are generated. Each instance uses an independent random seed, so multiple random nodes produce different sequences.

**See also:** $SN.control.midi$ -- the random mode generates a per-note random value on each note-on
