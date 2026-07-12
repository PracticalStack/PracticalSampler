---
title: Gain
description: "A gain module with decibel range and smoothed parameter changes."
factoryPath: core.gain
factory: core
polyphonic: true
tags: [core, gain, amplitude, utility]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors:
    - parameter: Smoothing
      impact: low
      note: "Per-sample processing while smoothing is active; efficient block multiply when settled"
forumReferences:
  - { tid: 1524, reason: "Width parameter mid/side behaviour clarification" }
seeAlso:
  - { id: "math.mul", type: alternative, reason: "Linear multiplication without dB scaling or smoothing" }
  - { id: "SimpleGain", type: module, reason: "Module-tree gain utility with smoothed dB control" }
commonMistakes:
  - title: "Gain range is attenuation only"
    wrong: "Expecting to boost a signal above unity with core.gain"
    right: "The Gain parameter ranges from -100 to 0 dB. Use math.mul for amplification above unity."
    explanation: "The maximum gain is 0 dB (unity). This node can only attenuate. For boosting, multiply the signal with a value greater than 1 using math.mul."
  - title: "Width amplifies the side channel, not a pseudo-stereo effect"
    wrong: "Using the Width parameter to add stereo width without monitoring output levels"
    right: "Width controls the gain of the side (L-R) signal. Values above 100% amplify the side channel and can push the output above the original level."
    explanation: "The Width parameter is a simple mid/side balance control. It does not apply any decorrelation or pseudo-stereo processing. Setting it beyond 100% boosts the side channel, which may cause clipping."
llmRef: |
  core.gain

  Applies a smoothed gain change in decibels to all channels. Attenuation only (0 dB max). Per-voice state in polyphonic mode with configurable reset value for voice starts.

  Signal flow:
    input * dBToLinear(Gain) -> output (all channels, smoothed)

  CPU: negligible, polyphonic

  Parameters:
    Gain (-100-0 dB, default 0): Target gain in decibels (attenuation only)
    Smoothing (0-1000 ms, default 20): Smoothing time for gain changes
    ResetValue (-100-0 dB, default 0): Instant gain applied on voice start before smoothing to target

  When to use:
    - Volume control and gain staging in any signal chain
    - Fade-in effects using ResetValue (e.g. ResetValue=-100dB, Gain=0dB)
    - The most common node in scriptnode networks (113 instances in usage survey)

  Common mistakes:
    - Cannot boost above 0 dB - use math.mul for amplification
    - Width parameter amplifies the side (L-R) channel; values above 100% can exceed original level

  Forum references: tid:1524 (Width parameter mid/side behaviour)

  See also:
    alternative math.mul -- linear multiplication without dB or smoothing
    [module] SimpleGain -- module-tree gain utility with smoothed dB control
---

The gain node multiplies all audio channels by a smoothed gain factor specified in decibels. It is the most commonly used node in scriptnode networks, providing clean volume control with built-in parameter smoothing to prevent clicks and zipper noise during gain changes.

The gain range is -100 to 0 dB (attenuation only). In polyphonic mode, each voice maintains its own smoothed gain value. The ResetValue parameter sets the initial gain when a voice starts, allowing fade-in effects: set ResetValue to -100 dB and Gain to 0 dB to create a smooth fade-in on each new note.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gain:
      desc: "Target gain in decibels (attenuation only)"
      range: "-100 - 0 dB"
      default: "0"
    Smoothing:
      desc: "Time for gain changes to ramp smoothly to the target"
      range: "0 - 1000 ms"
      default: "20"
    ResetValue:
      desc: "Gain applied instantly on voice start before smoothing to the Gain target"
      range: "-100 - 0 dB"
      default: "0"
  functions:
    dBToLinear:
      desc: "Converts a decibel value to a linear gain factor"
---

```
// core.gain - smoothed gain in decibels
// audio in -> audio out (all channels)

process(input) {
    target = dBToLinear(Gain)
    gainFactor = smooth(target, Smoothing)
    output = input * gainFactor    // all channels
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Gain
    params:
      - { name: Gain, desc: "Target gain in decibels. The range is attenuation only (0 dB = unity, -100 dB = near silence). The knob uses a skewed scale centred around typical mixing levels", range: "-100 - 0 dB", default: "0" }
      - { name: Smoothing, desc: "Time for gain changes to ramp to the target value. Prevents clicks and zipper noise. Set to 0 for instant changes", range: "0 - 1000 ms", default: "20" }
      - { name: ResetValue, desc: "Gain value applied instantly when a voice starts (or when the network is unbypassed), before smoothing to the current Gain target. Set to -100 dB for fade-in effects", range: "-100 - 0 dB", default: "0" }
---
::

### Smoothing behaviour

When the smoothing ramp is active, processing switches to per-sample mode. Once the gain has settled at its target, the node uses efficient block-based multiplication. This means short smoothing times have minimal CPU overhead.

### Width parameter

The Width parameter controls the gain of the side (L-R) signal in a mid/side decomposition. It does not apply decorrelation or pseudo-stereo processing. At 100%, the stereo image is unchanged. Below 100%, the side signal is attenuated (narrowing the image towards mono). Above 100%, the side signal is amplified, which increases the perceived stereo width but can push the output level above the original signal level.

### Limitations

At -100 dB the linear gain is approximately 0.00001, not exactly zero. For true silence, bypass the node or use a gate mechanism.

**See also:** $SN.math.mul$ -- linear multiplication without dB scaling or smoothing, $MODULES.SimpleGain$ -- module-tree gain utility with smoothed dB control
