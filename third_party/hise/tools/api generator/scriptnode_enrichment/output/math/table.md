---
title: Table
description: "A lookup table waveshaper that uses a 512-sample editable curve to transform the input signal."
factoryPath: math.table
factory: math
polyphonic: false
tags: [math, table, waveshaper, lookup, curve]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.pack", type: alternative, reason: "Lookup table using a SliderPack instead of a curve editor" }
  - { id: "core.peak", type: companion, reason: "Extracts envelope for use as table input" }
commonMistakes:
  - title: "Expecting audio-range input"
    wrong: "Feeding a bipolar audio signal (-1 to 1) directly into math.table"
    right: "Convert to the normalised range first using math.sig2mod, which maps -1..1 to 0..1."
    explanation: "The table expects input in the 0 to 1 range. Values outside this range are clamped, so a bipolar signal would use only the upper half of the table."
  - title: "Empty table passes signal through"
    wrong: "Expecting silence when no table is connected"
    right: "When no table data is connected, the signal passes through unmodified."
    explanation: "The node treats an empty table as a passthrough rather than outputting silence or zero."
llmRef: |
  math.table

  Lookup table waveshaper. Input is clamped to [0, 1], used as a normalised index into a 512-sample table, and linearly interpolated. Editable curve interface.

  Signal flow:
    audio in -> clamp [0, 1] -> table lookup (interpolated) -> audio out

  CPU: low, monophonic

  Parameters:
    None. Uses one Table complex data slot.

  When to use:
    Custom waveshaping curves, transfer functions, or any nonlinear mapping that is easier to draw than to express mathematically. Pair with core.peak or math.sig2mod to prepare the input range.

  Common mistakes:
    Input must be 0-1. Use math.sig2mod to convert bipolar audio signals.

  See also:
    [alternative] math.pack - SliderPack-based lookup table
    [companion] core.peak - envelope follower for preparing table input
---

A lookup table waveshaper that transforms the input signal through a 512-sample editable curve. The input is clamped to the 0 to 1 range and used as a normalised index into the table, with linear interpolation between adjacent entries. This makes it straightforward to draw arbitrary transfer functions, waveshaping curves, or custom response shapes.

The table is edited visually using the built-in curve editor. Changes to the table during playback are thread-safe - the audio thread reads the table data without blocking, so edits apply smoothly. If no table data is connected, the node passes the signal through unmodified.

## Signal Path

::signal-path
---
glossary:
  functions:
    clamp:
      desc: "Restricts input to the 0 to 1 range"
    tableLookup:
      desc: "Linearly interpolated lookup in the 512-sample table"
---

```
// math.table - lookup table waveshaper
// audio in -> audio out

process(input) {
    index = clamp(input, 0.0, 1.0)
    output = tableLookup(index)
}
```

::

The table always has 512 entries. For a lookup table with a different number of discrete steps, use [math.pack]($SN.math.pack$) with a SliderPack instead. Because the input is clamped to 0-1, bipolar audio signals need to be converted first -- use [math.sig2mod]($SN.math.sig2mod$) to shift a -1..1 signal into the 0..1 range before feeding it into the table.

**See also:** $SN.math.pack$ -- SliderPack-based lookup table, $SN.core.peak$ -- envelope follower for preparing table input
