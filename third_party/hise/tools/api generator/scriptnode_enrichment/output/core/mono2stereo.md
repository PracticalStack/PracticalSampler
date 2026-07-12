---
title: Mono2Stereo
description: "Copies the left channel to the right channel to create a dual-mono stereo signal."
factoryPath: core.mono2stereo
factory: core
polyphonic: false
tags: [core, mono, stereo, utility, routing]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Requires a stereo context"
    wrong: "Placing core.mono2stereo in a mono container and expecting it to add a second channel"
    right: "Place the node in a container that already has two channels. It copies channel data but does not add channels."
    explanation: "The node copies channel 0 to channel 1 but does not change the channel count. If only one channel exists, nothing happens."
llmRef: |
  core.mono2stereo

  Copies channel 0 (left) to channel 1 (right), creating a dual-mono stereo signal. Does not change the channel count.

  Signal flow:
    audio in [L, R] -> copy L to R -> audio out [L, L]

  CPU: negligible, monophonic

  Parameters:
    None

  When to use:
    Occasionally used (rank 70, 3 instances). Use after sound generators that only produce mono output on the left channel, to fill both channels before further stereo processing.

  Common mistakes:
    Requires a stereo context -- does not add channels, only copies between existing ones.

  See also:
    (none)
---

This node copies channel 0 (left) to channel 1 (right), overwriting whatever was previously in the right channel. It creates a dual-mono signal from a mono source. Most sound generators in scriptnode only produce output on the left channel, so this node provides a convenient way to fill both channels before applying stereo effects.

The node does not add channels or change the channel count. It must be placed in a container that already provides at least two channels. If only one channel is available, the node has no effect.

## Signal Path

::signal-path
---
glossary:
  functions:
    copyChannel:
      desc: "Copies all samples from channel 0 to channel 1"
---

```
// core.mono2stereo - duplicate left to right
// audio in [L, R] -> audio out [L, L]

process(left, right) {
    right = copyChannel(left)
}
```

::
