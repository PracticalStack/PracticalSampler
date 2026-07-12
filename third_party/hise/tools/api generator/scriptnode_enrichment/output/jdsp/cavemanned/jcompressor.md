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
    explanation: "Compressor does not include automatic or manual makeup gain. When signal exceeds threshold, output level is reduced. Use separate gain node to restore output level."
  - title: "Threshold at 0 dB means no compression"
    wrong: "Leaving Threshold at the default of 0 dB and expecting compression"
    right: "Lower Threshold to level above which compression should occur (e.g., -20 dB)."
    explanation: "Default threshold is 0 dB (maximum). Since audio signals rarely exceed 0 dBFS, compressor will not engage until threshold is lowered."
llmRef: |
  jdsp.jcompressor

  Lightweight compressor with four parameters: Threshold, Ratio, Attack, Release. Provides normalised gain reduction modulation output for UI visualisation. No makeup gain -- output quieter when compressing. Monophonic.

  Signal flow:
    audio in -> envelope detection (linked stereo) -> gain reduction -> audio out
    gain reduction -> modulation output (0-1, normalised)

  CPU: low, monophonic

  Parameters:
    Treshold (-100 - 0 dB, default 0) - compression threshold. Note: parameter name misspelled as "Treshold".
    Ratio (1 - 32, default 1) - compression ratio, minimum 1:1
    Attack (0 - 300 ms, default 1) - attack time of envelope follower
    Release (0 - 300 ms, default 100) - release time of envelope follower

  Modulation output:
    Sends normalised gain level (1.0 = no reduction, 0.0 = full reduction). Also drives display buffer for visualisation.

  When to use:
    Quick dynamics control where simple compressor is sufficient. For more features use dynamics.comp.

  Common mistakes:
    No makeup gain - add gain node after.
    Threshold defaults to 0 dB (no compression until lowered).

  See also:
    alternative dynamics.comp -- full-featured compressor
---

Lightweight compressor applying dynamic range compression to audio signal. Envelope detector uses linked stereo detection, so both channels are compressed by same amount based on combined signal level.

Modulation output sends current gain level as normalised value between 0 and 1, where 1.0 means no gain reduction and values approaching 0 indicate heavy compression. This output also drives display buffer, making it suitable for visualising compressor activity on UI element.

No makeup gain parameter. When compressor reduces signal level, output will be quieter. Add [gain]($SN.core.gain$) node after compressor to restore output level.

> [!Warning:Parameter name is misspelled] Threshold parameter is named `Treshold` (missing second 'h'). Use exact spelling when referencing parameter in scripts.

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
      desc: "Tracks signal level using linked stereo envelope follower"
    applyGain:
      desc: "Reduces signal gain based on threshold and ratio"
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
      - { name: Treshold, desc: "Level in dB above which compression is applied. Default is 0 dB (no compression). Lower value to engage compressor.", range: "-100 - 0 dB", default: "0" }
      - { name: Ratio, desc: "Amount of compression applied above threshold. Ratio of 4 means signal 4 dB above threshold is reduced to 1 dB above. Minimum is 1:1 (no compression).", range: "1 - 32", default: "1" }
  - label: Timing
    params:
      - { name: Attack, desc: "How quickly compressor responds when signal exceeds threshold. Shorter values catch transients; longer values let transients through.", range: "0 - 300 ms", default: "1" }
      - { name: Release, desc: "How quickly compressor stops compressing after signal drops below threshold.", range: "0 - 300 ms", default: "100" }
---
::

**See also:** [$SN.dynamics.comp$]($SN.dynamics.comp$) -- full-featured compressor with additional parameters and sidechain support
