---
title: Chain
description: "A serial container that processes each child node in sequence."
factoryPath: container.chain
factory: container
polyphonic: false
tags: [container, serial]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.split", type: alternative, reason: "Parallel processing instead of serial" }
  - { id: "container.modchain", type: disambiguation, reason: "Control-rate chain for modulation sources" }
  - { id: "container.midichain", type: disambiguation, reason: "Serial chain with sample-accurate MIDI event splitting" }
commonMistakes:
  - title: "Expecting click-free bypass transitions"
    wrong: "Bypassing a chain containing stateful effects (filters, delays) during playback"
    right: "Use container.soft_bypass for click-free bypass transitions with stateful effects."
    explanation: "Chain uses hard bypass - processing stops immediately with no crossfade. Stateful effects may produce clicks when abruptly bypassed or un-bypassed."
forumReferences:
  - { tid: 5825, reason: "Historical frame2block double-rendering bug (fixed May 2022)" }
llmRef: |
  container.chain

  A serial container that processes child nodes in sequence. Each child modifies the audio buffer in place and passes it to the next.

  Signal flow:
    input -> child[0] -> child[1] -> ... -> child[N] -> output

  CPU: negligible (pure dispatch), monophonic

  Parameters:
    None

  When to use:
    The default container for any serial signal path. Use for effect chains, processing pipelines, or any sequence of operations where each step feeds the next.

  Common mistakes:
    Chain uses hard bypass with no crossfade. Use container.soft_bypass for click-free transitions.

  See also:
    [alternative] container.split -- parallel processing instead of serial
    [disambiguation] container.modchain -- control-rate chain for modulation sources
    [disambiguation] container.midichain -- serial chain with MIDI event splitting
---

The chain is the fundamental building block of every scriptnode network. It processes child nodes one after another in list order - each child receives the output of the previous child and modifies the audio buffer in place.

When bypassed, no children are processed and audio passes through unmodified. This is a hard bypass with no crossfade, which may cause clicks with stateful effects such as filters or delays. For click-free bypass transitions, use [container.soft_bypass]($SN.container.soft_bypass$) instead.

## Signal Path

::signal-path
---
glossary:
  functions:
    serial dispatch:
      desc: "Passes the audio buffer through each child in list order"
---

```
// container.chain - serial processing
// audio in -> audio out

dispatch(input) {
    for each child in list order:
        child.process(input)    // in-place
}
```

::

The `IsVertical` property controls UI layout direction (vertical or horizontal) without affecting processing. MIDI events are forwarded to all children in list order; if a child modifies an event, subsequent children see the modified version.

**See also:** $SN.container.split$ -- parallel processing instead of serial, $SN.container.modchain$ -- control-rate chain for modulation sources, $SN.container.midichain$ -- serial chain with MIDI event splitting
