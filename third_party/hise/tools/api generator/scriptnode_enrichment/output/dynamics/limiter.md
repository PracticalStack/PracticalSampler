---
title: Limiter
description: "A peak limiter with fast envelope detection for transparent peak control."
factoryPath: dynamics.limiter
factory: dynamics
polyphonic: false
tags: [dynamics, limiter, sidechain]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "dynamics.comp", type: alternative, reason: "General-purpose compressor with standard envelope" }
  - { id: "dynamics.gate", type: alternative, reason: "Gate for attenuating below threshold" }
  - { id: "Dynamics", type: module, reason: "Module-tree dynamics processor that combines compressor, gate, and limiter" }
commonMistakes:
  - title: "Sidechain mode needs extra channels"
    wrong: "Enabling Sidechain mode on a stereo signal and expecting it to use an external key signal"
    right: "Route four channels into the node: channels 1-2 for audio, channels 3-4 for the sidechain key signal."
    explanation: "Sidechain mode splits the incoming channels in half. For stereo processing with an external key signal, the container must provide four channels."
  - title: "Set a high ratio for brickwall limiting"
    wrong: "Using Ratio 1 and expecting the limiter to catch peaks"
    right: "Set Ratio to a high value (16 or above) for effective peak limiting."
    explanation: "Ratio 1:1 means no gain reduction. The limiter only becomes effective at higher ratios, with very high values approaching brickwall behaviour."
  - title: "Latency varies with attack time"
    wrong: "Expecting zero latency from the limiter, or automating attack time during playback"
    right: "The limiter uses lookahead tied to the attack setting. Longer attack = more latency. Call Engine.setLatencySamples() with the maximum expected latency."
    explanation: "This is a lookahead limiter whose delay equals the attack time. The latency is not automatically reported to the DAW, so manual compensation is required. Changing the attack time during playback causes audible clicks and sudden delay changes."
  - title: "Do not automate attack time"
    wrong: "Modulating or automating the Attack parameter at runtime"
    right: "Treat Attack as a fixed setting. Set it once and do not change it during playback."
    explanation: "Changing the attack parameter at runtime causes an internal processing mode switch that introduces sudden large delay spikes and audible clicks. This is inherent to the lookahead design."
llmRef: |
  dynamics.limiter

  Peak limiter with fast envelope detection. Limits peaks above the threshold, controlled by ratio, attack, and release. Outputs inverse gain reduction as a normalised modulation signal.

  Signal flow:
    audio in -> peak detect -> limit (threshold, ratio, attack, release) -> audio out
    limit -> modulation out (1.0 - gain reduction)

  CPU: low, monophonic

  Parameters:
    Dynamics:
      Threshhold: -100 - 0 dB (default 0). Level above which limiting begins.
      Attack: 0 - 250 ms (default 50). How fast limiting engages.
      Release: 0 - 250 ms (default 50). How fast limiting releases.
      Ratio: 1 - 32 (default 1). Limiting ratio (high values approach brickwall).
    Routing:
      Sidechain: Disabled / Original / Sidechain (default Disabled). Detection source.

  When to use:
    Peak control and brickwall limiting. For general-purpose compression with a standard envelope, use dynamics.comp. For gating, use dynamics.gate.

  Common mistakes:
    Sidechain mode requires double the channel count (4 channels for stereo).
    Ratio 1:1 applies no limiting.
    Latency varies with attack time (lookahead design). Must call Engine.setLatencySamples() for DAW compensation.
    Do not automate attack time -- causes clicks and delay spikes.

  See also:
    [alternative] dynamics.comp - general-purpose compressor
    [alternative] dynamics.gate - gate for attenuating below threshold
    [module] Dynamics - module-tree dynamics processor that combines compressor, gate, and limiter
forumReferences:
  - { tid: 13300, summary: "Limiter lookahead latency and DAW compensation" }
  - { tid: 2857, summary: "Attack time changes cause clicks and delay spikes" }
---

A peak limiter that reduces gain when the input exceeds the threshold. The limiter uses fast envelope detection optimised for transparent peak control, making it more suitable for catching transients than the general-purpose [dynamics.comp]($SN.dynamics.comp$). At high ratio values the limiter approaches brickwall behaviour.

The limiter supports external sidechain keying and outputs inverse gain reduction as a normalised modulation signal.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Threshhold:
      desc: "Level above which limiting begins"
      range: "-100 - 0 dB"
      default: "0"
    Attack:
      desc: "How fast limiting engages"
      range: "0 - 250 ms"
      default: "50"
    Release:
      desc: "How fast limiting releases"
      range: "0 - 250 ms"
      default: "50"
    Ratio:
      desc: "Limiting ratio (high values approach brickwall)"
      range: "1 - 32"
      default: "1"
    Sidechain:
      desc: "Detection source selection"
      range: "Disabled / Original / Sidechain"
      default: "Disabled"
  functions:
    peakDetect:
      desc: "Measures the peak level of the input or sidechain signal"
    limit:
      desc: "Applies gain reduction to peaks above threshold with fast envelope detection"
---

```
// dynamics.limiter - peak limiter
// audio in -> audio out + modulation out

process(input) {
    level = peakDetect(input, Sidechain)
    gain = limit(level, Threshhold, Ratio, Attack, Release)
    output = input * gain
    modulation = 1.0 - gainReduction  // 1.0 = no limiting, 0.0 = full limiting
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Dynamics
    params:
      - { name: Threshhold, desc: "Level above which limiting begins.", range: "-100 - 0 dB", default: "0" }
      - { name: Attack, desc: "How quickly limiting engages after the signal exceeds the threshold. Also determines the lookahead delay. Do not automate -- changing at runtime causes clicks.", range: "0 - 250 ms", default: "50" }
      - { name: Release, desc: "How quickly limiting releases after the signal drops below the threshold.", range: "0 - 250 ms", default: "50" }
      - { name: Ratio, desc: "Limiting ratio. Higher values produce stronger peak control, approaching brickwall limiting.", range: "1 - 32", default: "1" }
  - label: Routing
    params:
      - { name: Sidechain, desc: "Detection source. Disabled and Original both use the input signal. Sidechain uses extra channels as the key signal.", range: "Disabled / Original / Sidechain", default: "Disabled" }
---
::

### Lookahead Latency

The limiter uses a lookahead design where the delay equals the attack time setting. A longer attack means more lookahead and more latency. This latency is not automatically reported to the DAW host, so you must call `Engine.setLatencySamples()` with the maximum expected latency to ensure correct DAW latency compensation and aligned offline rendering.

Do not change the Attack parameter during playback. Modifying it at runtime causes an internal processing mode switch that introduces sudden delay spikes and audible clicks. Treat Attack as a fixed setting.

### Modulation Output

The modulation output sends the inverse of the gain reduction, normalised to 0..1. A value of 1.0 means no limiting is occurring; lower values indicate more gain reduction.

**See also:** $SN.dynamics.comp$ -- general-purpose compressor with standard envelope, $SN.dynamics.gate$ -- gate for attenuating below threshold, $MODULES.Dynamics$ -- module-tree dynamics processor that combines compressor, gate, and limiter
