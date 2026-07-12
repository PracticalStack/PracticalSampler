---
title: Bipolar Mod
description: "A bipolar modulation source template that converts a signal into a bipolar offset around a base value with adjustable intensity."
factoryPath: template.bipolar_mod
factory: template
polyphonic: false
tags: [template, modulation, control]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.pma", type: companion, reason: "The output stage used internally for multiply-add" }
  - { id: "control.bipolar", type: companion, reason: "The internal bipolar scaler" }
commonMistakes:
  - title: "Forgetting to connect pma output"
    wrong: "Dropping the template into a network and expecting it to modulate something automatically"
    right: "Drag a modulation cable from the pma node's output to the target parameter you want to modulate."
    explanation: "The template has no default modulation target. The pma node's output must be manually connected to the parameter you want to control."
  - title: "Leaving the dummy ramp in place"
    wrong: "Using the template with the default 1 Hz ramp as the modulation source"
    right: "Replace the dummy ramp inside the mod_signal chain with your actual modulation source."
    explanation: "The default core.ramp is a placeholder for testing. Replace it with your desired modulation source such as an LFO, envelope follower, or MIDI controller."
llmRef: |
  template.bipolar_mod

  A composite template that provides a bipolar modulation source. Converts a user-supplied signal into a bipolar offset around a base value, with adjustable intensity.

  Signal flow:
    mod_signal (user source) -> sig2mod (scale to 0..1) -> peak (capture value) -> bipolar (scale to -Intensity..+Intensity) -> pma (Value + offset) -> modulation output

  CPU: negligible, monophonic
    Runs entirely at control rate. All internal nodes are lightweight control/math operations.

  Parameters:
    Value (0.0 - 1.0, default 0.0): Base/centre value for the modulation output. When Intensity is 0, the output equals this value.
    Intensity (0.0 - 1.0, default 0.0): Modulation depth. At 0, no bipolar offset is applied. At 1, the full range is added.

  When to use:
    Use when you need a modulation source with a tuneable centre point and adjustable depth. Typical applications include vibrato, tremolo, or any parameter modulation that should oscillate around a user-defined base value.

  Common mistakes:
    The pma output must be manually connected to a target parameter. The dummy ramp must be replaced with an actual modulation source.

  See also:
    [companion] control.pma -- the output stage used internally
    [companion] control.bipolar -- the internal bipolar scaler
---

This template provides a ready-made bipolar modulation source. It takes a signal from a user-supplied source, converts it to a 0..1 range, then applies a bipolar scaling around a base value. The result is a modulation output that oscillates between `Value - Intensity` and `Value + Intensity`, suitable for driving any target parameter.

The template runs entirely at control rate and contains a placeholder [ramp]($SN.core.ramp$) inside the `mod_signal` chain. Replace this dummy source with your actual modulation signal (LFO, envelope follower, expression input, etc.), then connect the [pma]($SN.control.pma$) node's modulation output to the parameter you want to modulate.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Base/centre value for the modulation output"
      range: "0.0 - 1.0"
      default: "0.0"
    Intensity:
      desc: "Modulation depth, controls the bipolar scaling range"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    sig2mod:
      desc: "Converts audio range (-1..1) to modulation range (0..1)"
    bipolar scale:
      desc: "Maps 0..1 input to a bipolar offset in the range -Intensity..+Intensity"
    pma combine:
      desc: "Adds the bipolar offset to the base Value: output = Value + offset"
---

```
// template.bipolar_mod - bipolar modulation source
// control signal in -> modulation output

process() {
    signal = mod_signal.generate()      // user-replaceable source
    normalised = sig2mod(signal)         // -1..1 -> 0..1
    offset = bipolar scale(normalised, Intensity)  // 0..1 -> -Intensity..+Intensity
    output = pma combine(Value, offset)  // Value + offset
    // -> connect output to target parameter
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Modulation
    params:
      - { name: Value, desc: "Base/centre value for the modulation output. When Intensity is 0, the output equals this value.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Intensity, desc: "Modulation depth. At 0, no offset is applied. At 1, the full bipolar range is added to the base value.", range: "0.0 - 1.0", default: "0.0" }
---
::

The `mod_signal` chain expects a source that produces a signal in the -1..1 audio range. The internal sig2mod stage converts this to 0..1 before bipolar scaling. The entire template runs at control rate (decimated sample rate), so it is not suitable for audio-rate modulation that requires sample-accurate timing. The internal pma node's Multiply parameter is fixed at 1.0 and not exposed, simplifying the formula to `output = Value + offset`.

**See also:** $SN.control.pma$ -- the output stage used internally, $SN.control.bipolar$ -- the internal bipolar scaler
