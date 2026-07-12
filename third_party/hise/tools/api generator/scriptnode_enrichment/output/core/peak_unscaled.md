---
title: Peak Unscaled
description: "Measures the peak input value preserving sign and sends it as an unnormalised modulation signal."
factoryPath: core.peak_unscaled
factory: core
polyphonic: false
tags: [core, peak, analysis, modulation, unscaled]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "core.peak", type: disambiguation, reason: "Normalised variant that folds negative values to positive" }
commonMistakes:
  - title: "Output can be negative"
    wrong: "Connecting core.peak_unscaled to a parameter that expects a 0-1 range"
    right: "Use core.peak for normalised 0-1 output, or connect to parameters via control.pma_unscaled for proper range handling."
    explanation: "The output preserves the sign of the loudest sample and bypasses target parameter range scaling. A negative input peak produces a negative modulation value."
llmRef: |
  core.peak_unscaled

  Measures the peak input value per block, preserving the sign of the sample with the largest absolute value. Sends the result as an unnormalised modulation signal. The audio passes through unmodified.

  Signal flow:
    audio in -> peak detection (signed, max abs across channels) -> modulation out (raw)
    audio in -> audio out (passthrough)

  CPU: negligible, monophonic

  Parameters:
    None

  When to use:
    Unused in surveyed networks. Use when you need the raw signed peak value (e.g. for DC offset detection or bipolar envelope following). For standard envelope followers, prefer core.peak.

  Common mistakes:
    Output can be negative -- not suitable for parameters expecting 0-1 range.

  See also:
    [disambiguation] core.peak -- normalised variant with absolute folding
---

This node analyses the input signal and detects the sample with the largest absolute value across all channels for each audio block, preserving its sign. The result is sent as an unnormalised modulation signal, meaning the modulation output bypasses the normal 0-1 range scaling when driving target parameters. The audio signal passes through completely unmodified.

Unlike [core.peak]($SN.core.peak$), which folds negative values to positive (absolute), this variant returns the actual signed value. A signal peaking at -0.8 produces a modulation output of -0.8 rather than 0.8. This makes it suitable for scenarios where the polarity of the signal matters, such as DC offset detection or bipolar control signals.

The display graph automatically adjusts its range to show whatever values are being processed, since the output is not constrained to a fixed range. The modulation output updates once per audio block, just like [core.peak]($SN.core.peak$); wrap the node in a fixed-block container for predictable update rates.

This node also supports a display buffer for UI visualisation using the [DisplayBuffer]($API.DisplayBuffer$) scripting API.

## Signal Path

::signal-path
---
glossary:
  functions:
    peakDetectSigned:
      desc: "Finds the sample with the largest absolute value across all channels, preserving its sign"
---

```
// core.peak_unscaled - signed peak to raw modulation signal
// audio in -> modulation out (raw, can be negative)

analyse(input) {
    peak = peakDetectSigned(input)  // signed value with largest abs
    output = peak                    // sent as unnormalised modulation
    // audio passes through unchanged
}
```

::

**See also:** $SN.core.peak$ -- normalised variant that folds negative values to positive
