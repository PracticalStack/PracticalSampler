---
title: Goniometer
description: "A stereo correlation display that plots left and right channels as an X-Y Lissajous figure."
factoryPath: analyse.goniometer
factory: analyse
polyphonic: false
tags: [analyse, goniometer, stereo, correlation, lissajous]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "analyse.fft", type: alternative, reason: "Frequency-domain spectrum display instead of stereo field" }
  - { id: "analyse.oscilloscope", type: alternative, reason: "Time-domain waveform display instead of stereo field" }
  - { id: "Analyser", type: module, reason: "Stereo goniometer display" }
commonMistakes:
  - title: "Requires stereo input"
    wrong: "Placing the goniometer in a mono signal path and expecting a display"
    right: "Place the goniometer after a point in the chain where two channels are available."
    explanation: "The node requires exactly two channels. In a mono context it has no meaningful stereo data to display."
llmRef: |
  analyse.goniometer

  Stereo correlation display (Lissajous X-Y plot). Writes left and right channels to a ring buffer for visualisation. Audio passes through unmodified. Requires stereo (2 channels).

  Signal flow:
    stereo in (L/R) -> ring buffer write -> stereo out (unchanged)

  CPU: negligible, monophonic

  Parameters:
    None.

  Display buffer properties:
    BufferLength: 512 - 32768 samples.

  When to use:
    Checking stereo width, phase correlation, and spatial balance during sound design. Not observed in surveyed networks (design-time tool).

  Common mistakes:
    Requires stereo input -- meaningless in a mono context.

  See also:
    [alternative] analyse.fft - frequency spectrum display
    [alternative] analyse.oscilloscope - time-domain waveform display
    [module] Analyser - module-tree analyser with goniometer mode
---

Displays the stereo correlation of the audio signal as an X-Y Lissajous plot. The left channel maps to one axis and the right channel to the other, giving a visual representation of the stereo field. Audio passes through the node completely unmodified.

A mono (summed) signal appears as a diagonal line. A fully anti-phase signal appears as a perpendicular line. Uncorrelated stereo content fills the display plane. This makes the goniometer useful for checking stereo width, phase issues, and spatial balance during sound design.

## Signal Path

::signal-path
---
glossary:
  functions:
    writeToRingBuffer:
      desc: "Copies both stereo channels into the display ring buffer for X-Y visualisation"
---

```
// analyse.goniometer - stereo correlation display
// stereo in -> stereo out (unchanged)

analyse(left, right) {
    writeToRingBuffer(left, right)
    output = input    // both channels pass through
}
```

::

### Display Buffer Configuration

The ring buffer length is configurable between 512 and 32768 samples via the display buffer properties. Longer buffers show more history but respond more slowly to changes.

### UI Integration

To display the goniometer on your main user interface, register the display buffer as an external [DisplayBufferSource]($API.DisplayBufferSource$) and render the output from script.

**See also:** $SN.analyse.fft$ -- frequency spectrum display, $SN.analyse.oscilloscope$ -- time-domain waveform display, $MODULES.Analyser$ -- module-tree analyser with goniometer mode
