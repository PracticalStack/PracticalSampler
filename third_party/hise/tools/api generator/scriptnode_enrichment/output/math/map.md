---
title: Map
description: "Maps an input signal from one value range to another using linear interpolation with clamping."
factoryPath: math.map
factory: math
polyphonic: false
tags: [math, map, range, scale, normalise]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.bipolar", type: companion, reason: "Converts a unipolar signal to a bipolar range" }
  - { id: "control.normaliser", type: companion, reason: "Normalises a parameter value to 0-1" }
commonMistakes:
  - title: "Expecting extrapolation beyond the input range"
    wrong: "Feeding values outside [InputStart, InputEnd] and expecting the output to extend beyond [OutputStart, OutputEnd]"
    right: "Values outside the input range are clamped. The output never exceeds the bounds set by OutputStart and OutputEnd."
    explanation: "The node clamps the input before mapping. If you need extrapolation, use a math.expr node with a custom formula instead."
  - title: "Equal InputStart and InputEnd"
    wrong: "Setting InputStart and InputEnd to the same value and expecting a passthrough"
    right: "When InputStart equals InputEnd, the output is always OutputStart regardless of input."
    explanation: "The node guards against division by zero by setting the scale factor to zero, so the output collapses to a constant."
llmRef: |
  math.map

  Linear range mapper. Remaps a signal from [InputStart, InputEnd] to [OutputStart, OutputEnd] with clamping (no extrapolation). Inverted ranges are supported.

  Signal flow:
    audio in -> subtract InputStart -> clamp -> scale and offset -> audio out

  CPU: negligible, monophonic

  Parameters:
    InputStart: 0.0 - 1.0 (default 0.0). Start of expected input range.
    InputEnd: 0.0 - 1.0 (default 1.0). End of expected input range.
    OutputStart: 0.0 - 1.0 (default 0.0). Start of output range.
    OutputEnd: 0.0 - 1.0 (default 1.0). End of output range.

  When to use:
    Rescaling a signal or modulation value from one range to another. Useful for adapting control signals that arrive in a different range than the target parameter expects.

  Common mistakes:
    Input is clamped - no extrapolation beyond the defined ranges.
    Equal InputStart and InputEnd produces a constant output.

  See also:
    [companion] control.bipolar - unipolar to bipolar conversion
    [companion] control.normaliser - normalise parameter values
---

Maps an input signal from one value range to another using a linear function. The four parameters define the source range (InputStart to InputEnd) and the destination range (OutputStart to OutputEnd). Input values are clamped to the source range before mapping, so the output is always bounded by OutputStart and OutputEnd.

Inverted ranges work correctly - setting OutputStart higher than OutputEnd produces an inverted output. This can be useful for creating inverse control relationships, such as mapping a brightness knob so that higher values reduce filter cutoff.

## Signal Path

::signal-path
---
glossary:
  parameters:
    InputStart:
      desc: "Start of the expected input range"
      range: "0.0 - 1.0"
      default: "0.0"
    InputEnd:
      desc: "End of the expected input range"
      range: "0.0 - 1.0"
      default: "1.0"
    OutputStart:
      desc: "Start of the output range"
      range: "0.0 - 1.0"
      default: "0.0"
    OutputEnd:
      desc: "End of the output range"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    clamp:
      desc: "Restricts the shifted value to the input range length, preventing extrapolation"
---

```
// math.map - linear range mapper
// audio in -> audio out

process(input) {
    shifted = input - InputStart
    clamped = clamp(shifted, 0, abs(InputEnd - InputStart))
    output = clamped * (OutputEnd - OutputStart) / (InputEnd - InputStart) + OutputStart
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Input Range
    params:
      - { name: InputStart, desc: "Start of the expected input range. Values below this are clamped.", range: "0.0 - 1.0", default: "0.0" }
      - { name: InputEnd, desc: "End of the expected input range. Values above this are clamped.", range: "0.0 - 1.0", default: "1.0" }
  - label: Output Range
    params:
      - { name: OutputStart, desc: "Start of the output range. Mapped from InputStart.", range: "0.0 - 1.0", default: "0.0" }
      - { name: OutputEnd, desc: "End of the output range. Mapped from InputEnd.", range: "0.0 - 1.0", default: "1.0" }
---
::

**See also:** $SN.control.bipolar$ -- unipolar to bipolar conversion, $SN.control.normaliser$ -- normalise parameter values
