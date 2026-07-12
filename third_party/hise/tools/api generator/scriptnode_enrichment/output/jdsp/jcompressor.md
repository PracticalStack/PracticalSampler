---
title: Compressor
description: "A simple compressor with threshold, ratio, attack, and release controls, plus a gain reduction modulation output."
factoryPath: jdsp.jcompressor
factory: jdsp
polyphonic: false
tags: [jdsp, dynamics, compressor]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "dynamics.comp", type: alternative, reason: "Full-featured compressor with additional parameters and sidechain support" }
commonMistakes:
  - title: "No built-in makeup gain"
    wrong: "Expecting the compressor to maintain output level after compression"
    right: "Add a gain node after the compressor to compensate for the level reduction."
    explanation: "This compressor does not include automatic or manual makeup gain. When the signal exceeds the threshold, the output level is reduced. Use a separate gain node to restore the output level."
  - title: "Threshold at 0 dB means no compression"
    wrong: "Leaving Threshold at the default of 0 dB and expecting compression"
    right: "Lower the Threshold to the level above which compression should occur (e.g., -20 dB)."
    explanation: "The default threshold is 0 dB (maximum). Since audio signals rarely exceed 0 dBFS, the compressor will not engage until the threshold is lowered."
llmRef: |
  jdsp.jcompressor

  A lightweight compressor with four parameters: Threshold, Ratio, Attack, and Release. Provides a normalised gain reduction modulation output for UI visualisation. No makeup gain -- output is quieter when compressing. Monophonic.

  Signal flow:
    audio in -> envelope detection (linked stereo) -> gain reduction -> audio out
    gain reduction -> modulation output (0-1, normalised)

  CPU: low, monophonic

  Parameters:
    Treshold (-100 - 0 dB, default 0) - compression threshold. Note: parameter name is misspelled as "Treshold".
    Ratio (1 - 32, default 1) - compression ratio, minimum 1:1
    Attack (0 - 300 ms, default 1) - attack time of the envelope follower
    Release (0 - 300 ms, default 100) - release time of the envelope follower

  Modulation output:
    Sends normalised gain level (1.0 = no reduction, 0.0 = full reduction). Also drives the display buffer for visualisation.

  When to use:
    Quick dynamics control where a simple compressor is sufficient. For more features use dynamics.comp.

  Common mistakes:
    No makeup gain - add a gain node after.
    Threshold defaults to 0 dB (no compression until lowered).

  See also:
    alternative dynamics.comp -- full-featured compressor
---

A lightweight compressor that applies dynamic range compression to the audio signal. The envelope detector uses linked stereo detection, so both channels are compressed by the same amount based on the combined signal level.

The modulation output sends the current gain level as a normalised value between 0 and 1, where 1.0 means no gain reduction and values approaching 0 indicate heavy compression. This output also drives a display buffer, making it suitable for visualising the compressor's activity on a UI element.

There is no makeup gain parameter. When the compressor reduces the signal level, the output will be quieter. Add a [gain]($SN.core.gain$) node after the compressor to restore the output level.

> [!Warning:Parameter name is misspelled] The threshold parameter is named `Treshold` (missing the second 'h'). Use this exact spelling when referencing the parameter in scripts.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Treshold:
      desc: "Compression threshold in dB"
      range: "-100 - 0 dB"
      default: "0"
    Ratio:
      desc: "Compression ratio (minimum 1:1)"
      range: "1 - 32"
      default: "1"
    Attack:
      desc: "Envelope follower attack time"
      range: "0 - 300 ms"
      default: "1"
    Release:
      desc: "Envelope follower release time"
      range: "0 - 300 ms"
      default: "100"
  functions:
    detectLevel:
      desc: "Tracks the signal level using a linked stereo envelope follower"
    applyGain:
      desc: "Reduces the signal gain based on the threshold and ratio"
---

```
// jdsp.jcompressor - dynamics compression
// audio in -> audio out + gain reduction modulation

process(input) {
    level = detectLevel(input, Attack, Release)

    if level > Treshold:
        reduction = (level - Treshold) * (1 - 1/Ratio)
        output = applyGain(input, -reduction)
    else:
        output = input

    modulationOutput = 1.0 - gainReduction  // normalised 0-1
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Detection
    params:
      - { name: Treshold, desc: "The level in dB above which compression is applied. Default is 0 dB (no compression). Lower the value to engage the compressor.", range: "-100 - 0 dB", default: "0" }
      - { name: Ratio, desc: "The amount of compression applied above the threshold. A ratio of 4 means a signal 4 dB above the threshold is reduced to 1 dB above. Minimum is 1:1 (no compression).", range: "1 - 32", default: "1" }
  - label: Timing
    params:
      - { name: Attack, desc: "How quickly the compressor responds when the signal exceeds the threshold. Shorter values catch transients; longer values let transients through.", range: "0 - 300 ms", default: "1" }
      - { name: Release, desc: "How quickly the compressor stops compressing after the signal drops below the threshold.", range: "0 - 300 ms", default: "100" }
---
::

**See also:** [$SN.dynamics.comp$]($SN.dynamics.comp$) -- full-featured compressor with additional parameters and sidechain support
