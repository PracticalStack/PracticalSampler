---
title: Updown Comp
description: "A dual-threshold compressor with upward compression below LowThreshold and downward compression above HighThreshold."
factoryPath: dynamics.updown_comp
factory: dynamics
polyphonic: false
tags: [dynamics, compressor, upward, downward, dual-threshold]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors:
    - { parameter: "RMS", impact: "slight increase", note: "RMS adds per-sample buffer processing" }
seeAlso:
  - { id: "dynamics.comp", type: alternative, reason: "Simpler downward-only compressor" }
commonMistakes:
  - title: "Fixed stereo processing only"
    wrong: "Using dynamics.updown_comp in a mono or multi-channel context"
    right: "Ensure the signal path provides exactly two channels (stereo)."
    explanation: "This node is fixed to two-channel processing. It will not work correctly with mono or multi-channel signal paths."
  - title: "LowThreshold must be below HighThreshold"
    wrong: "Setting LowThreshold above HighThreshold and expecting normal behaviour"
    right: "Keep LowThreshold below HighThreshold to maintain a unity zone between them."
    explanation: "The transfer function expects a low-to-high threshold order. The knee width is also clamped to half the distance between thresholds, so overlapping thresholds collapse the unity zone."
llmRef: |
  dynamics.updown_comp

  Dual-threshold compressor operating on fixed stereo signals. Applies upward compression below LowThreshold and downward compression above HighThreshold, with unity gain between. Supports soft knee and optional RMS detection.

  Signal flow:
    stereo in -> peak detect -> optional RMS (30ms) -> envelope follower -> dual-threshold transfer -> gain apply -> stereo out
    gain apply -> modulation out (gain ratio 0..1)

  CPU: medium, monophonic

  Parameters:
    Thresholds:
      LowThreshold: -100 - 0 dB (default -100). Below this, upward compression boosts.
      LowRatio: 0.2 - 100 (default 1). Upward compression ratio.
      HighThreshold: -100 - 0 dB (default 0). Above this, downward compression reduces.
      HighRatio: 0.2 - 100 (default 1). Downward compression ratio.
    Envelope:
      Knee: 0 - 0.3 (default 0.15). Soft knee width for both thresholds.
      Attack: 0 - 1000 ms (default 50). Envelope attack time.
      Release: 0 - 1000 ms (default 50). Envelope release time.
    Detection:
      RMS: Off / On (default On). Off = peak detection, On = 30ms RMS window.

  When to use:
    Narrowing dynamic range from both ends: boosting quiet signals and taming loud ones. Use dynamics.comp for simple downward-only compression.

  Common mistakes:
    Fixed stereo only - does not work with mono or multi-channel.
    LowThreshold must be below HighThreshold.

  See also:
    [alternative] dynamics.comp - simpler downward-only compressor
---

A dual-threshold compressor that narrows the dynamic range from both ends. Signals below the LowThreshold are boosted by upward compression, signals above the HighThreshold are reduced by downward compression, and signals between the two thresholds pass at unity gain. This makes it well suited for evening out dynamics without losing the natural feel of the source material.

The node operates on fixed stereo signals and includes a soft knee for smooth transitions around both thresholds. An optional RMS detector smooths transient peaks for a more average-level-based compression response. All gain-domain parameters are smoothed to prevent clicks when values change.

## Signal Path

::signal-path
---
glossary:
  parameters:
    LowThreshold:
      desc: "Threshold for upward compression"
      range: "-100 - 0 dB"
      default: "-100"
    LowRatio:
      desc: "Upward compression ratio"
      range: "0.2 - 100"
      default: "1"
    HighThreshold:
      desc: "Threshold for downward compression"
      range: "-100 - 0 dB"
      default: "0"
    HighRatio:
      desc: "Downward compression ratio"
      range: "0.2 - 100"
      default: "1"
    Knee:
      desc: "Soft knee width applied to both thresholds"
      range: "0 - 0.3"
      default: "0.15"
    Attack:
      desc: "Envelope attack time"
      range: "0 - 1000 ms"
      default: "50"
    Release:
      desc: "Envelope release time"
      range: "0 - 1000 ms"
      default: "50"
    RMS:
      desc: "Detection mode: Off = peak, On = 30ms RMS"
      range: "Off / On"
      default: "On"
  functions:
    peakDetect:
      desc: "Computes the maximum absolute sample value across both channels"
    rmsSmooth:
      desc: "Smooths the detected level using a 30ms running RMS window"
    dualTransfer:
      desc: "Applies the dual-threshold transfer function: upward below low, unity between, downward above high"
---

```
// dynamics.updown_comp - dual-threshold compressor
// stereo in -> stereo out + modulation out

process(left, right) {
    peak = peakDetect(left, right)

    if (RMS == On)
        level = rmsSmooth(peak)
    else
        level = peak

    level = followEnvelope(level, Attack, Release)
    target = dualTransfer(level, LowThreshold, LowRatio, HighThreshold, HighRatio, Knee)
    gain = target / level

    left  *= gain
    right *= gain
    modulation = clamp(gain, 0, 1)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Thresholds
    params:
      - { name: LowThreshold, desc: "Threshold for upward compression. Signals below this level are boosted towards it.", range: "-100 - 0 dB", default: "-100" }
      - { name: LowRatio, desc: "Upward compression ratio. Higher values boost quiet signals more aggressively. 1 = no upward compression.", range: "0.2 - 100", default: "1" }
      - { name: HighThreshold, desc: "Threshold for downward compression. Signals above this level are reduced towards it.", range: "-100 - 0 dB", default: "0" }
      - { name: HighRatio, desc: "Downward compression ratio. Higher values reduce loud signals more aggressively. 1 = no downward compression.", range: "0.2 - 100", default: "1" }
  - label: Envelope
    params:
      - { name: Knee, desc: "Soft knee width applied to both thresholds. Higher values create smoother transitions. 0 = hard knee.", range: "0 - 0.3", default: "0.15" }
      - { name: Attack, desc: "How quickly the envelope responds to increasing level.", range: "0 - 1000 ms", default: "50" }
      - { name: Release, desc: "How quickly the envelope responds to decreasing level.", range: "0 - 1000 ms", default: "50" }
  - label: Detection
    params:
      - { name: RMS, desc: "Detection mode. Off uses peak detection for transient-sensitive response. On uses a 30ms RMS window for average-level compression.", range: "Off / On", default: "On" }
---
::

### Noise Floor

The node includes a noise floor at -82 dB that prevents the upward compressor from boosting silence. Signals below this level are not amplified, avoiding the common problem of raising the noise floor with upward compression.

### Modulation Output

The modulation output sends the applied gain ratio clamped to 0..1. Values below 1.0 indicate downward compression. Upward gain boost (values above 1.0) is clamped to 1.0 in the modulation output, so the modulation signal only reflects attenuation, not boost.

**See also:** $SN.dynamics.comp$ -- simpler downward-only compressor
