---
title: Multi
description: "A parallel container that assigns each child a different slice of the audio channels."
factoryPath: container.multi
factory: container
polyphonic: false
tags: [container, parallel, multichannel]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.split", type: alternative, reason: "Same channels to each child instead of different channels" }
  - { id: "container.sidechain", type: alternative, reason: "Channel doubling for sidechain input instead of channel splitting" }
commonMistakes:
  - title: "More children than available channels"
    wrong: "Adding three child nodes in a stereo (2-channel) context"
    right: "Ensure the channel count is at least equal to the number of children. Use a container with more channels or reduce children."
    explanation: "Multi divides channels equally among children. If there are more children than channels, an error is raised and excess children are silently skipped."
forumReferences:
  - { tid: 7457, reason: "HISE_NUM_FX_PLUGIN_CHANNELS for multi-output plugin export" }
llmRef: |
  container.multi

  A parallel container that splits the audio channels among its children. Each child processes a non-overlapping slice of channels. The total channel count equals the sum of all children's channel counts.

  Signal flow:
    input (ch0..chN) -> child[0](ch0..chK) | child[1](chK+1..chM) | ... -> output (ch0..chN)

  CPU: negligible (pointer offset only), monophonic

  Parameters:
    None

  When to use:
    Independent left/right processing, mid/side processing (with routing.ms_encode/ms_decode), or any topology requiring per-channel processing paths.

  Plugin export:
    Set HISE_NUM_FX_PLUGIN_CHANNELS in ExtraDefinitions to expose more than 2 channels from a compiled FX plugin. Internal channel count can exceed this value for hidden processing channels.

  Common mistakes:
    Adding more children than available channels raises an error. Ensure channel count >= child count.

  See also:
    [alternative] container.split -- same channels to each child instead of different channels
    [alternative] container.sidechain -- channel doubling for sidechain input
---

The multi container splits the audio channels among its children. In a stereo context with two children, child 0 processes the left channel and child 1 processes the right channel. Each child modifies its channel slice in place with no buffer copying or summing required.

Channels are divided equally during preparation. For mid/side processing, place a [routing.ms_encode]($SN.routing.ms_encode$) node before the multi container and a [routing.ms_decode]($SN.routing.ms_decode$) node after it. Each child then processes the mid or side signal independently.

## Signal Path

::signal-path
---
glossary:
  functions:
    channel split:
      desc: "Assigns a non-overlapping slice of channels to each child"
---

```
// container.multi - per-channel processing
// audio in (N channels) -> audio out (N channels)

dispatch(input) {
    offset = 0
    for each child:
        channels = child's channel count
        channel split: child.process(input[offset .. offset + channels])
        offset += channels
}
```

::

Each child receives an independent copy of MIDI events, so one child's event modifications do not affect others.

### Multi-Channel Plugin Export

To expose more than one stereo pair from an FX plugin that uses a multi container (or any multichannel master container), set the `HISE_NUM_FX_PLUGIN_CHANNELS` preprocessor define in ExtraDefinitions to the desired channel count. Without this, the compiled plugin only exposes 2 channels regardless of the internal channel configuration. This also enables DAW sidechain inputs on supporting hosts. You can set the internal routing matrix channel count higher than `HISE_NUM_FX_PLUGIN_CHANNELS` to use extra channel pairs for internal processing without exposing them to the DAW.

**See also:** $SN.container.split$ -- same channels to each child instead of different channels, $SN.container.sidechain$ -- channel doubling for sidechain input
