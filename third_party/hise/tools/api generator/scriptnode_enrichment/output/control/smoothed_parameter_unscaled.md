---
title: Smoothed Parameter (Unscaled)
description: "Smoothes an incoming parameter value with unnormalised output, sending the raw value to targets."
factoryPath: control.smoothed_parameter_unscaled
factory: control
polyphonic: true
tags: [control, smoothing, parameter, unscaled]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.smoothed_parameter", type: disambiguation, reason: "Same smoothing with normalised 0-1 output" }
  - { id: "control.unscaler", type: companion, reason: "Simple passthrough for unnormalised values without smoothing" }
commonMistakes:
  - title: "Expecting normalised 0-1 output"
    wrong: "Connecting smoothed_parameter_unscaled to a target that expects normalised input"
    right: "Use smoothed_parameter_unscaled only when the target expects the raw value without range conversion"
    explanation: "This variant sends the raw Value directly. The target parameter's range mapping is bypassed, so the received value is used as-is."
llmRef: |
  control.smoothed_parameter_unscaled

  Smoothes an incoming parameter value with unnormalised output. Identical to smoothed_parameter except the output sends raw values rather than normalised 0-1.

  Signal flow:
    Control node - processes in the audio callback
    Value -> smoother -> unnormalised modulation output (raw value)

  CPU: low, polyphonic (per-voice smoothing state)

  Parameters:
    Value (0.0 - 1.0, default 0.0): Target value for the smoother (unscaled input)
    SmoothingTime (0.1 - 1000.0 ms, default 100.0): Duration of the smoothing ramp
    Enabled (Off / On, default On): Toggles smoothing; when off, value passes through instantly

  Properties:
    Mode: Linear Ramp | Low Pass | No

  When to use:
    Use when you need smoothed parameter changes and the target expects raw values rather than normalised 0-1 input. Common with frequency, time, or gain parameters that operate in specific units.

  Common mistakes:
    Expecting normalised output -- this variant bypasses range conversion

  See also:
    [disambiguation] control.smoothed_parameter -- same smoothing with normalised 0-1 output
    [companion] control.unscaler -- simple passthrough for unnormalised values without smoothing
---

Smoothed Parameter (Unscaled) applies time-based smoothing to an incoming parameter value and outputs the raw value without normalisation. It is identical to [control.smoothed_parameter]($SN.control.smoothed_parameter$) in every respect except that the modulation output bypasses the target parameter's range conversion, sending the value as-is.

Use this variant when the target parameter expects a raw value in specific units (such as frequency in Hz or time in milliseconds) rather than a normalised 0-1 signal. The Mode property offers the same three smoothing algorithms: Linear Ramp, Low Pass, and No.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Target value for the smoother (sent as raw unnormalised output)"
      range: "0.0 - 1.0"
      default: "0.0"
    SmoothingTime:
      desc: "Duration of the smoothing ramp"
      range: "0.1 - 1000.0 ms"
      default: "100.0"
    Enabled:
      desc: "Toggles smoothing on or off"
      range: "Off / On"
      default: "On"
  functions:
    advance:
      desc: "Steps the smoother forward by one sample, returning the current interpolated value"
---

```
// control.smoothed_parameter_unscaled - smooth parameter transitions (raw output)
// control in -> unnormalised control out

onValueChange(input) {
    smoother.setTarget(Value)
}

perSample {
    smoothed = advance(SmoothingTime)

    if Enabled:
        output = smoothed   // gradual ramp, raw value
    else:
        output = Value      // instant passthrough, raw value
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Target value for the smoother. The modulation output sends this raw value without range conversion.", range: "0.0 - 1.0", default: "0.0" }
  - label: Configuration
    params:
      - { name: SmoothingTime, desc: "Duration of the smoothing ramp.", range: "0.1 - 1000.0 ms", default: "100.0" }
      - { name: Enabled, desc: "Toggles smoothing. When off, Value passes through instantly with no ramp.", range: "Off / On", default: "On" }
---
::

The smoothing modes (Linear Ramp, Low Pass, No) behave identically to [control.smoothed_parameter]($SN.control.smoothed_parameter$). The only difference is the output: this variant bypasses normalisation, so the target receives the exact value rather than having it mapped through the target's parameter range. Each voice maintains its own independent smoothing state in polyphonic contexts.

**See also:** $SN.control.smoothed_parameter$ -- same smoothing with normalised 0-1 output, $SN.control.unscaler$ -- simple passthrough for unnormalised values without smoothing
