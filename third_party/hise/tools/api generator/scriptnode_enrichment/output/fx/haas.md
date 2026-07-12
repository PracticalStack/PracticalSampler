---
title: Haas
description: "Stereo positioning effect using inter-channel delay (Haas effect) with up to 20ms offset."
factoryPath: fx.haas
factory: fx
polyphonic: true
tags: [fx, stereo, panning]
screenshot: /images/v2/reference/scriptnodes/fx/haas.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "jdsp.jpanner", type: alternative, reason: "Amplitude-based panning versus Haas delay-based panning" }
commonMistakes:
  - title: "Requires stereo input signal"
    wrong: "Using fx.haas on a mono channel expecting stereo widening"
    right: "Place fx.haas after a mono-to-stereo conversion (e.g. core.mono2stereo) so it receives two channels."
    explanation: "The node requires exactly two input channels. With mono input the behaviour is undefined. Ensure the signal is stereo before this node."
llmRef: |
  fx.haas

  Stereo positioning effect using the Haas psychoacoustic principle. Delays one channel by up to 20ms relative to the other, creating the perception of directional placement without amplitude changes.

  Signal flow:
    stereo in -> delay one channel (0-20ms based on Position) -> stereo out

  CPU: low, polyphonic

  Parameters:
    Position (-1.0 - 1.0, default 0.0) - stereo position. Negative = sound from left (right channel delayed), positive = sound from right (left channel delayed), 0 = centre (no delay).

  When to use:
    Natural-sounding stereo placement that preserves mono compatibility better than amplitude panning. Effective for positioning elements in the stereo field without altering their level.

  Common mistakes:
    Requires stereo input - mono signals need conversion first.

  See also:
    alternative jdsp.jpanner - amplitude-based panning
---

Stereo positioning effect based on the Haas psychoacoustic principle. One channel is delayed by up to 20ms relative to the other, creating the perception that the sound originates from the direction of the earlier-arriving channel. Unlike amplitude-based panning, the Haas effect preserves equal levels in both channels.

The Position parameter controls which channel is delayed and by how much. At Position=0, both channels pass through with no delay. Positive values delay the left channel (sound appears from the right); negative values delay the right channel (sound appears from the left).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Position:
      desc: "Stereo position from left (-1) through centre (0) to right (+1)"
      range: "-1.0 - 1.0"
      default: "0.0"
  functions:
    delay:
      desc: "Applies a short delay (0-20ms) to one channel based on position"
---

```
// fx.haas - stereo delay panning
// stereo in -> stereo out

process(left, right) {
    delayMs = abs(Position) * 20ms

    if Position > 0:
        left  = delay(left, delayMs)    // delay left -> sound from right
        right = right                    // right unchanged
    else if Position < 0:
        left  = left                     // left unchanged
        right = delay(right, delayMs)    // delay right -> sound from left
    // Position == 0: both pass through
}
```

::

## Parameters

::parameter-table
---
groups:
  - label:
    params:
      - { name: Position, desc: "Stereo position. Negative values place the sound to the left by delaying the right channel; positive values place it to the right by delaying the left channel. The delay scales linearly from 0ms at centre to 20ms at the extremes.", range: "-1.0 - 1.0", default: "0.0" }
---
::

### Limitations

The maximum delay is 20ms, which is within the psychoacoustic range where the brain fuses two arrivals into a single perceived source with directional information. Beyond roughly 30ms, the delayed channel begins to be heard as a separate echo.

The node requires a stereo input (exactly two channels). Additional channels beyond the first two are ignored.

**See also:** $SN.jdsp.jpanner$ -- amplitude-based panning alternative
