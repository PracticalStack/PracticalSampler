---
title: Smoothed Parameter
description: "Smoothes an incoming parameter value to prevent abrupt changes, with selectable smoothing modes."
factoryPath: control.smoothed_parameter
factory: control
polyphonic: true
tags: [control, smoothing, parameter]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: "SmoothingTime", impact: "negligible", note: "Only affects ramp duration, not per-sample cost" }
seeAlso:
  - { id: "control.smoothed_parameter_unscaled", type: disambiguation, reason: "Same smoothing with unnormalised output" }
  - { id: "core.smoother", type: alternative, reason: "Audio-rate signal smoother" }
commonMistakes:
  - title: "Confusing normalised and unscaled variants"
    wrong: "Using control.smoothed_parameter when the target expects a raw value (e.g. frequency in Hz)"
    right: "Use control.smoothed_parameter_unscaled for raw values, control.smoothed_parameter for normalised 0-1 output"
    explanation: "The standard variant outputs normalised values (0-1) which are scaled by the target parameter's range. The unscaled variant sends the raw value directly."
llmRef: |
  control.smoothed_parameter

  Smoothes an incoming parameter value over time to prevent clicks and zipper noise. Supports linear ramp, low-pass, and bypass modes.

  Signal flow:
    Control node - processes in the audio callback
    Value -> smoother -> normalised modulation output (0-1)

  CPU: low, polyphonic (per-voice smoothing state)

  Parameters:
    Value (0.0 - 1.0, default 0.0): Target value for the smoother
    SmoothingTime (0.1 - 1000.0 ms, default 100.0): Duration of the smoothing ramp
    Enabled (Off / On, default On): Toggles smoothing; when off, value passes through instantly

  Properties:
    Mode: Linear Ramp | Low Pass | No

  When to use:
    Use to smooth parameter changes that would otherwise cause audible artefacts. Place between a parameter source and an audio-processing target.

  Common mistakes:
    Confusing normalised and unscaled variants -- use smoothed_parameter_unscaled for raw values

  See also:
    [disambiguation] control.smoothed_parameter_unscaled -- same smoothing with unnormalised output
    [alternative] core.smoother -- audio-rate signal smoother
---

Smoothed Parameter applies time-based smoothing to an incoming parameter value, preventing clicks and zipper noise when parameters change. The smoothed output updates every sample or block in the audio callback, gradually ramping towards the target value.

The Mode property selects the smoothing algorithm: Linear Ramp interpolates at a constant rate, Low Pass uses an exponential curve that approaches the target asymptotically, and No bypasses smoothing entirely. Each voice maintains its own smoothing state in polyphonic contexts.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Target value for the smoother"
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
// control.smoothed_parameter - smooth parameter transitions
// control in -> normalised control out

onValueChange(input) {
    smoother.setTarget(Value)
}

perSample {
    smoothed = advance(SmoothingTime)

    if Enabled:
        output = smoothed   // gradual ramp
    else:
        output = Value      // instant passthrough
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Target value for the smoother. The output gradually ramps towards this value.", range: "0.0 - 1.0", default: "0.0" }
  - label: Configuration
    params:
      - { name: SmoothingTime, desc: "Duration of the smoothing ramp.", range: "0.1 - 1000.0 ms", default: "100.0" }
      - { name: Enabled, desc: "Toggles smoothing. When off, Value passes through instantly with no ramp.", range: "Off / On", default: "On" }
---
::

### Smoothing Modes

The Mode property must be set before compilation and determines the smoothing algorithm:

- **Linear Ramp** -- constant-rate interpolation, reaches the target in exactly the specified time
- **Low Pass** -- exponential approach, fast initial response that tapers off near the target
- **No** -- no smoothing, equivalent to setting Enabled to off permanently

On voice start, the smoother resets to the current target value and forces a modulation update, ensuring the first sample of each voice receives the correct value.

**See also:** $SN.control.smoothed_parameter_unscaled$ -- same smoothing with unnormalised output, $SN.core.smoother$ -- audio-rate signal smoother
