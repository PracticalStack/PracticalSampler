---
title: MS Encode
description: "Converts a stereo left/right signal to mid/side representation."
factoryPath: routing.ms_encode
factory: routing
polyphonic: false
tags: [routing, mid-side, stereo, encoding]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.ms_decode", type: companion, reason: "Converts mid/side back to left/right -- always used together" }
llmRef: |
  routing.ms_encode

  Converts a stereo L/R signal to mid/side representation. M = (L+R)*0.5, S = (L-R)*0.5. Channel 0 becomes Mid, channel 1 becomes Side. The encode/decode pair is gain-neutral. Non-stereo signals pass through unmodified.

  Signal flow:
    stereo in (L/R) -> MS encode -> stereo out (M/S)

  CPU: negligible, monophonic

  Parameters:
    None.

  When to use:
    Used in 13 networks (rank 32). Always paired with routing.ms_decode. Place before a container.multi to process mid and side channels independently (e.g. widening effects, mid-only EQ), then decode after.

  See also:
    [companion] routing.ms_decode - converts mid/side back to left/right
---

Converts a stereo left/right signal to mid/side representation. After encoding, channel 0 contains the mid signal (sum of left and right) and channel 1 contains the side signal (difference of left and right). The encoding formula uses 0.5 scaling so that the round-trip through [routing.ms_decode]($SN.routing.ms_decode$) is gain-neutral.

Place this node before a [container.multi]($SN.container.multi$) to process mid and side channels independently -- for example, applying EQ only to the mid channel or adjusting stereo width by scaling the side channel. Follow with routing.ms_decode to convert back to left/right.

The 0.5 scaling factor is applied during encoding, not decoding. This means the mid and side channels are at half the amplitude of the original left/right channels. The decode node compensates by using no scaling factor, so the full round-trip is gain-neutral.

Non-stereo signals (mono or more than two channels) pass through unmodified.

## Signal Path

::signal-path
---
glossary:
  functions:
    msEncode:
      desc: "Converts left/right to mid/side: M = (L+R)*0.5, S = (L-R)*0.5"
---

```
// routing.ms_encode - stereo to mid/side
// stereo in (L/R) -> stereo out (M/S)

process(left, right) {
    mid  = (left + right) * 0.5
    side = (left - right) * 0.5
    output = [mid, side]
}
```

::

**See also:** $SN.routing.ms_decode$ -- converts mid/side back to left/right
