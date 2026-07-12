---
title: Change
description: "Filters out repeated values, only forwarding the signal when it actually changes."
factoryPath: control.change
factory: control
polyphonic: true
tags: [control, change, filter, duplicate, gate]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.pma_unscaled", type: companion, reason: "Often used after change for additional scaling of the filtered signal" }
commonMistakes:
  - title: "First value of 0.0 is suppressed"
    wrong: "Expecting the first incoming value to always trigger an output"
    right: "The stored value starts at 0.0. If the first incoming value is also 0.0, no output is sent. Send a non-zero value first, or be aware of this initial state."
    explanation: "The change detection compares against a stored value that starts at 0.0. Only values that differ from the stored value are forwarded. There is no special first-value-always-passes logic."
llmRef: |
  control.change

  Filters out repeated values. Only forwards the signal when the incoming value differs from the previously stored value. Uses exact floating-point comparison.

  Signal flow:
    Control node - no audio processing
    Value (raw) -> change detection -> modulation out (unnormalised, only when changed)

  CPU: negligible, polyphonic

  Parameters:
    Value (unscaled, default 0.0): input signal with change detection

  When to use:
    Use to reduce unnecessary parameter updates when a source repeatedly sends the same value. Common after core.peak or other analysis nodes that may output the same value on consecutive blocks.

  Common mistakes:
    First value of 0.0 is suppressed because the initial stored value is 0.0.

  See also:
    [companion] control.pma_unscaled -- additional scaling after filtering
---

Filters out repeated values from a control signal, only forwarding the value when it actually changes. This is useful for reducing unnecessary parameter updates when a source such as [core.peak]($SN.core.peak$) repeatedly sends the same value on consecutive processing blocks.

The comparison uses exact floating-point equality. Values that differ by even the smallest representable amount will pass through, while only bit-identical values are suppressed. Both input and output are unscaled -- the raw value is forwarded without any transformation.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Raw input signal with change detection applied"
      range: "unscaled"
      default: "0.0"
  functions:
    changeDetect:
      desc: "Compares incoming value to stored value; forwards only when different"
---

```
// control.change - duplicate value filter
// control in (raw) -> control out (raw, only when changed)

onValueChange(input) {
    if (Value != previousValue)
        output = Value
    previousValue = Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Input signal. Receives unscaled values. Only forwarded to the modulation output when the value differs from the previous one.", range: "unscaled", default: "0.0" }
---
::

Each voice maintains its own stored value for comparison when used in a polyphonic context. A value that is "new" for one voice may be "same" for another.

**See also:** $SN.control.pma_unscaled$ -- additional scaling after filtering
