---
title: PMA Unscaled
description: "Scales and offsets a raw modulation signal without clamping or range conversion."
factoryPath: control.pma_unscaled
factory: control
polyphonic: true
tags: [control, pma, unscaled, multiply, add, raw]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.pma", type: disambiguation, reason: "Normalised variant that clamps output to 0-1" }
  - { id: "control.minmax", type: alternative, reason: "Maps a normalised input to a custom range with skew and step options" }
commonMistakes:
  - title: "Multiply receives range-scaled input"
    wrong: "Connecting a modulation source to Multiply and expecting it to receive the raw value"
    right: "Only Value and Add receive raw values. Multiply receives a range-scaled value converted through its -1 to 1 range."
    explanation: "Value and Add are marked as unscaled inputs, so they receive raw values from connected sources. Multiply is not unscaled, so it receives a normalised value mapped through its -1 to 1 range."
llmRef: |
  control.pma_unscaled

  Scales and offsets a raw modulation signal. Computes Value * Multiply + Add with no clamping. Both Value and Add receive raw (unscaled) values; Multiply is range-scaled.

  Signal flow:
    Control node - no audio processing
    Value (raw) -> * Multiply -> + Add (raw) -> modulation out (unnormalised)

  CPU: negligible, polyphonic

  Parameters:
    Value (unscaled, default 0.0): raw input signal
    Multiply (-1.0 - 1.0, default 1.0): scale factor (range-scaled input)
    Add (unscaled, default 0.0): raw offset added after multiplication

  When to use:
    Frequently used (rank 15, 22 instances). Use when you need to scale or offset values in their native domain (Hz, ms, semitones, etc.) without normalisation. Common for arithmetic on frequency, delay time, or pitch values.

  Common mistakes:
    Multiply receives range-scaled input, not raw values like Value and Add.

  See also:
    [disambiguation] control.pma -- normalised variant with 0-1 clamping
    [alternative] control.minmax -- maps normalised input to a custom range
---

The PMA Unscaled node performs the same multiply-add operation as [control.pma]($SN.control.pma$) but works with raw, unnormalised values. It computes `Value * Multiply + Add` without clamping, and the output bypasses the connection system's range conversion. This makes it suitable for arithmetic on values in their native domain such as frequencies in Hz, delay times in milliseconds, or pitch in semitones.

Both the Value and Add parameters receive raw values from connected sources without range conversion. The Multiply parameter, however, is range-scaled through its -1 to 1 range, so a connected modulation source is mapped into that range before being used as the scale factor.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Primary input receiving raw values from the connected source"
      range: "unscaled"
      default: "0.0"
    Multiply:
      desc: "Scale factor, range-scaled through -1 to 1"
      range: "-1.0 - 1.0"
      default: "1.0"
    Add:
      desc: "Raw offset added after multiplication"
      range: "unscaled"
      default: "0.0"
---

```
// control.pma_unscaled - raw multiply-add
// control in (raw) -> control out (raw)

onValueChange(input) {
    output = Value * Multiply + Add
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Raw input signal. Receives unscaled values from the connected source.", range: "unscaled", default: "0.0" }
  - label: Transform
    params:
      - { name: Multiply, desc: "Scale factor applied to the input. This parameter is range-scaled, so connected modulation sources are mapped through the -1 to 1 range.", range: "-1.0 - 1.0", default: "1.0" }
      - { name: Add, desc: "Raw offset added after multiplication. Receives unscaled values from connected sources.", range: "unscaled", default: "0.0" }
---
::

### Limitations

The output has no overflow protection. If the connected sources produce extreme values, the result can be any value including very large numbers. Ensure that downstream nodes can handle the expected output range.

**See also:** $SN.control.pma$ -- normalised variant that clamps output to 0-1, $SN.control.minmax$ -- maps a normalised input to a custom range
