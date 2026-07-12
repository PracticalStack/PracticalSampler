---
title: MS Decode
description: "Converts a mid/side signal back to stereo left/right."
factoryPath: routing.ms_decode
factory: routing
polyphonic: false
tags: [routing, mid-side, stereo, decoding]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.ms_encode", type: companion, reason: "Converts left/right to mid/side -- always used together" }
llmRef: |
  routing.ms_decode

  Converts a mid/side signal back to stereo left/right. L = M + S, R = M - S. Channel 0 becomes Left, channel 1 becomes Right. The encode/decode pair is gain-neutral. Non-stereo signals pass through unmodified.

  Signal flow:
    stereo in (M/S) -> MS decode -> stereo out (L/R)

  CPU: negligible, monophonic

  Parameters:
    None.

  When to use:
    Used in 13 networks (rank 31). Always paired with routing.ms_encode. Place after mid/side processing to convert back to left/right.

  See also:
    [companion] routing.ms_encode - converts left/right to mid/side
---

Converts a mid/side signal back to stereo left/right. Channel 0 (mid) and channel 1 (side) are recombined into left and right channels. The decoding uses no scaling factor, which compensates for the 0.5 scaling applied during [routing.ms_encode]($SN.routing.ms_encode$), making the full round-trip gain-neutral.

This node must be paired with [routing.ms_encode]($SN.routing.ms_encode$). Using ms_decode on a signal that was not encoded will produce unexpected results -- channel 0 would be treated as mid and channel 1 as side regardless of their actual content.

Place this node after any mid/side processing to restore the standard left/right stereo format. Non-stereo signals pass through unmodified.

## Signal Path

::signal-path
---
glossary:
  functions:
    msDecode:
      desc: "Converts mid/side back to left/right: L = M + S, R = M - S"
---

```
// routing.ms_decode - mid/side to stereo
// stereo in (M/S) -> stereo out (L/R)

process(mid, side) {
    left  = mid + side
    right = mid - side
    output = [left, right]
}
```

::

**See also:** $SN.routing.ms_encode$ -- converts left/right to mid/side
