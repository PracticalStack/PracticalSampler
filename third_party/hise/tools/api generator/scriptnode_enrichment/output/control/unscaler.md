---
title: Unscaler
description: "Forwards a parameter value as an unnormalised modulation signal, bypassing the target's range conversion."
factoryPath: control.unscaler
factory: control
polyphonic: false
tags: [control, unscaled, passthrough]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.normaliser", type: disambiguation, reason: "Same passthrough but with normalised output (target range is applied)" }
  - { id: "control.smoothed_parameter_unscaled", type: companion, reason: "Unnormalised output with smoothing" }
llmRef: |
  control.unscaler

  Forwards the raw parameter value as an unnormalised modulation signal. The target receives the exact value without range conversion.

  Signal flow:
    Control node - no audio processing
    Value -> direct passthrough -> unnormalised modulation output

  CPU: negligible, monophonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): Input value forwarded unchanged as unnormalised output

  When to use:
    Use when a parameter source provides a value in specific units (e.g. Hz, ms) and the target should receive that exact value without normalisation or range mapping.

  See also:
    [disambiguation] control.normaliser -- same passthrough with normalised output
    [companion] control.smoothed_parameter_unscaled -- unnormalised output with smoothing
---

Unscaler forwards a parameter value directly to its modulation output without any transformation, bypassing the target parameter's range conversion. The target receives the exact value as-is, making this node useful when a value in specific units (such as frequency or time) needs to pass through without being mapped to the target's range.

This is the unnormalised counterpart to [control.normaliser]($SN.control.normaliser$). With normaliser, the target applies its own range mapping to the incoming 0-1 value. With unscaler, the target uses the raw value directly.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Input value forwarded unchanged to the output"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// control.unscaler - raw value passthrough
// control in -> unnormalised control out

onValueChange(input) {
    output = Value  // no range conversion
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: ""
    params:
      - { name: Value, desc: "Input value forwarded unchanged as an unnormalised modulation signal. The target receives this exact value without range conversion.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.control.normaliser$ -- same passthrough with normalised output (target range is applied), $SN.control.smoothed_parameter_unscaled$ -- unnormalised output with smoothing
