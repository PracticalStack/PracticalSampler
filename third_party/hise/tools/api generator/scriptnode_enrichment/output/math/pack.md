---
title: Pack
description: "A lookup table that uses a SliderPack as its data source, with linear interpolation between slider values."
factoryPath: math.pack
factory: math
polyphonic: false
tags: [math, pack, sliderpack, lookup, sequencer]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "math.table", type: alternative, reason: "Lookup table using a 512-sample editable curve instead" }
  - { id: "core.peak", type: companion, reason: "Extracts envelope for use as lookup input" }
commonMistakes:
  - title: "Confusing input scaling with math.table"
    wrong: "Assuming math.pack and math.table handle input identically"
    right: "Both expect 0-1 input, but math.pack scales the input by the number of sliders internally. An input of 0.0 reads the first slider; an input of 1.0 reads the last."
    explanation: "The input-to-index mapping differs: math.table normalises across a fixed 512-sample range, while math.pack scales by the current number of sliders. The practical effect is the same - 0 maps to the start and 1 maps to the end - but the resolution depends on how many sliders the pack contains."
llmRef: |
  math.pack

  SliderPack lookup table. Input (0-1) is scaled by the number of sliders, clamped, and linearly interpolated between adjacent slider values. Variable resolution.

  Signal flow:
    audio in -> scale by pack size -> clamp -> interpolated lookup -> audio out

  CPU: low, monophonic

  Parameters:
    None. Uses one SliderPack complex data slot.

  When to use:
    Step sequencer patterns, discrete value mappings, or lookup tables where individual steps need to be visible and editable. Prefer math.table when a smooth continuous curve is needed.

  Common mistakes:
    Input scaling differs from math.table - pack scales by slider count internally.

  See also:
    [alternative] math.table - continuous curve lookup table
    [companion] core.peak - envelope follower for preparing input
---

A lookup table that uses a SliderPack as its data source. The input signal (expected in the 0 to 1 range) is scaled by the number of sliders, clamped, and linearly interpolated between adjacent slider values. This makes it well suited for step sequencer patterns, discrete value mappings, or any transfer function where individual steps should be visible and independently editable.

The number of sliders determines the resolution of the lookup. An input of 0.0 reads the first slider value; an input of 1.0 reads the last. Unlike [math.table]($SN.math.table$), which always has 512 entries, the pack size is determined by the connected SliderPack and can be changed freely. If no SliderPack data is connected, the signal passes through unmodified.

## Signal Path

::signal-path
---
glossary:
  functions:
    scaleByPackSize:
      desc: "Multiplies the input by the number of sliders in the pack"
    clamp:
      desc: "Restricts the scaled index to valid bounds"
    packLookup:
      desc: "Linearly interpolated lookup between adjacent slider values"
---

```
// math.pack - slider pack lookup table
// audio in -> audio out

process(input) {
    index = scaleByPackSize(input)
    index = clamp(index, 0, packSize - 1)
    output = packLookup(index)
}
```

::

Linear interpolation is applied between adjacent slider values, producing smooth transitions even with a small number of sliders. If you want hard steps without interpolation, you will need to use closely spaced identical values in adjacent sliders. The pack reads are thread-safe -- editing slider values during playback applies changes smoothly without blocking the audio thread.

**See also:** $SN.math.table$ -- continuous curve lookup table, $SN.core.peak$ -- envelope follower for preparing input
