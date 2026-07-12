---
title: Table
description: "A symmetrical lookup table that reads the input signal magnitude and displays the result."
factoryPath: core.table
factory: core
polyphonic: false
tags: [core, table, waveshaper, lookup]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.table", type: alternative, reason: "Lookup table applied to the control signal domain" }
commonMistakes:
  - title: "Audio signal passes through unchanged"
    wrong: "Expecting core.table to act as a waveshaper that modifies the audio signal"
    right: "The audio passes through unmodified. The table lookup result is used for display only."
    explanation: "Despite the description mentioning waveshaping, this node reads the input magnitude for visualisation but does not alter the audio. Use a custom SNEX node or math.tanh for actual waveshaping."
llmRef: |
  core.table

  A symmetrical lookup table node. Reads the absolute input magnitude, maps it through a user-editable table, and displays the result. The audio signal passes through unmodified.

  Signal flow:
    audio in -> abs(sample) -> table lookup -> display
    audio in -> audio out (passthrough)

  CPU: negligible, monophonic

  Parameters:
    None

  When to use:
    Unused in surveyed networks. Use when you need to visualise a table-mapped version of the input signal magnitude. Not suitable for audio waveshaping.

  See also:
    [alternative] math.table -- lookup table in the control signal domain
---

This node reads the absolute value of the input audio signal, maps it through a user-editable 512-point lookup table, and displays the result. The audio signal itself passes through completely unmodified. The table lookup is symmetrical: positive and negative input values of the same magnitude map to the same table position.

The lookup table is editable through the node's UI. The input range of 0 to 1 (after taking the absolute value) maps across the full table. The node has no parameters -- the table curve is edited directly through the graphical interface. The table uses linear interpolation between its 512 entries for smooth lookup results.

## Signal Path

::signal-path
---
glossary:
  functions:
    abs:
      desc: "Takes the absolute value of the input sample"
    tableLookup:
      desc: "Interpolated lookup into the 512-entry user-editable table"
---

```
// core.table - symmetrical table lookup for display
// audio in -> audio out (passthrough)

process(input) {
    value = abs(input)
    display = tableLookup(value)
    // audio passes through unchanged
}
```

::

**See also:** $SN.math.table$ -- lookup table for control signals
