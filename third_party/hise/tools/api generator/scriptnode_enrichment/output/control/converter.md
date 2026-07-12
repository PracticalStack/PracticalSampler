---
title: Converter
description: "Converts a control value between unit domains using one of 14 predefined conversion formulas."
factoryPath: control.converter
factory: control
polyphonic: false
tags: [control, converter, units, frequency, milliseconds, pitch, gain]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.normaliser", type: alternative, reason: "Normalises a value using range conversion rather than unit conversion" }
  - { id: "control.pma_unscaled", type: companion, reason: "Often used before or after converter for additional scaling" }
llmRef: |
  control.converter

  Converts a raw input value between unit domains using one of 14 predefined formulas selected by the Mode property. Both input and output are unscaled.

  Signal flow:
    Control node - no audio processing
    Value (raw) -> conversion formula (selected by Mode) -> modulation out (unnormalised)

  CPU: negligible, monophonic

  Parameters:
    Value (unscaled, default 0.0): raw input value in the source domain

  Properties:
    Mode: selects the conversion formula (Ms2Freq, Freq2Ms, Freq2Samples, Ms2Samples, Samples2Ms, Ms2BPM, Pitch2St, St2Pitch, Pitch2Cent, Cent2Pitch, Midi2Freq, Freq2Norm, Gain2dB, dB2Gain)

  When to use:
    Very commonly used (rank 14, 27 instances). Use when you need to convert a value from one unit domain to another, such as milliseconds to frequency, semitones to pitch factor, or gain to decibels.

  See also:
    [alternative] control.normaliser -- range-based normalisation
    [companion] control.pma_unscaled -- additional scaling before or after conversion
---

Converts a raw input value from one unit domain to another using a predefined formula selected by the Mode property. Both the input and output are unscaled -- the node receives raw values and sends raw converted values without any range normalisation. This is the most commonly used control node for unit conversion in scriptnode networks.

The Mode property selects one of 14 conversion formulas. Three of them (Freq2Samples, Ms2Samples, Samples2Ms) depend on the current sample rate and update automatically when the sample rate changes.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Raw input value in the source unit domain"
      range: "unscaled"
      default: "0.0"
  functions:
    convert:
      desc: "Applies the selected unit conversion formula"
---

```
// control.converter - unit domain conversion
// control in (raw) -> control out (raw)

onValueChange(input) {
    output = convert(Value)   // formula selected by Mode
}
```

::

## Mode Reference

| Mode | Conversion | Input | Output |
|------|-----------|-------|--------|
| Ms2Freq | 1000 / input | Milliseconds | Hz |
| Freq2Ms | 1000 / input | Hz | Milliseconds |
| Freq2Samples | sampleRate / input | Hz | Samples |
| Ms2Samples | input * 0.001 * sampleRate | Milliseconds | Samples |
| Samples2Ms | input / sampleRate * 1000 | Samples | Milliseconds |
| Ms2BPM | 60000 / input | Milliseconds | BPM |
| Pitch2St | log2(input) * 12 | Pitch factor | Semitones |
| St2Pitch | 2^(input / 12) | Semitones | Pitch factor |
| Pitch2Cent | log2(input) * 1200 | Pitch factor | Cents |
| Cent2Pitch | 2^(input / 1200) | Cents | Pitch factor |
| Midi2Freq | MIDI note to Hz | 0-1 normalised | Hz |
| Freq2Norm | input / 20000 | Hz | 0-1 normalised |
| Gain2dB | linear to dB | Linear gain | dB |
| dB2Gain | dB to linear | dB | Linear gain |

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Raw input value in the source unit domain. Receives unscaled values from connected sources.", range: "unscaled", default: "0.0" }
---
::

This node is monophonic and does not maintain per-voice state. The Midi2Freq mode is an exception -- it expects a normalised 0-1 input (which it multiplies by 127 internally to get a MIDI note number). All other modes expect values in their stated input domain. Modes that involve division (Ms2Freq, Freq2Ms, Ms2BPM) protect against division by zero.

**See also:** $SN.control.normaliser$ -- range-based normalisation, $SN.control.pma_unscaled$ -- additional scaling before or after conversion
