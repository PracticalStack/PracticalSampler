---
title: Normaliser
description: "Passes an unscaled input value through as a normalised modulation signal, allowing the connection system to apply the target's range."
factoryPath: control.normaliser
factory: control
polyphonic: false
tags: [control, normaliser, scaling, normalised, range]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.minmax", type: companion, reason: "The inverse operation -- converts normalised 0-1 to an unscaled range" }
  - { id: "control.converter", type: alternative, reason: "Converts between specific unit domains rather than normalising" }
commonMistakes:
  - title: "Set the Value parameter range"
    wrong: "Connecting an unscaled source and leaving the Value parameter's range at its default 0-1"
    right: "Set the range of the Value parameter to match the expected input domain (e.g. 20-20000 for frequency in Hz) so the normalisation maps correctly."
    explanation: "The normaliser works by accepting a value through the Value parameter and letting the connection system map it using the parameter's range. If the range does not match the source's output domain, the normalised result will be incorrect."
llmRef: |
  control.normaliser

  Passes a value through unchanged. The normalisation happens in the connection layer -- the output is treated as normalised 0-1, so the target parameter's range conversion is applied.

  Signal flow:
    Control node - no audio processing
    Value (0..1 via range) -> pass-through -> modulation out (normalised)

  CPU: negligible, monophonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): input -- set this parameter's range to match the source domain

  When to use:
    Commonly used (rank 22, 18 instances). Use when you need to connect an unscaled modulation source to a target that expects normalised input. Set the Value parameter's range to match the source domain, and the connection system handles the rest.

  Common mistakes:
    Must set the Value parameter's range to match the source domain for correct normalisation.

  See also:
    [companion] control.minmax -- inverse operation, normalised to unscaled
    [alternative] control.converter -- converts between specific unit domains
---

Converts an unscaled modulation signal to a normalised 0-1 output by passing the value through the connection system's range conversion. The node itself applies no transformation -- it simply forwards the input to its modulation output. Because the output is marked as normalised, the connection system applies the target parameter's range, converting the 0-1 value into the target's domain.

To use this node, connect the unscaled modulation source to the Value parameter and set that parameter's range to match the source's output domain. For example, if the source sends frequency values between 20 and 20000 Hz, set the Value parameter's range accordingly. The incoming raw value is then normalised to 0-1 by the input range, and the connection system scales it to the target parameter's range on output.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Input value -- set this parameter's range to match the source domain"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// control.normaliser - normalised pass-through
// control in -> control out (normalised)

onValueChange(input) {
    output = Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Input signal. Set this parameter's range to match the source's output domain so the value is correctly normalised to 0-1.", range: "0.0 - 1.0", default: "0.0" }
---
::

This node is monophonic and does not maintain per-voice state. The [control.minmax]($SN.control.minmax$) node performs the inverse operation: it takes a normalised 0-1 input and maps it to a custom unscaled range.

**See also:** $SN.control.minmax$ -- inverse operation converting normalised to unscaled, $SN.control.converter$ -- converts between specific unit domains
