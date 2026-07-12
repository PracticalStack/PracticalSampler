---
title: No MIDI
description: "A serial container that blocks all MIDI events from reaching its children."
factoryPath: container.no_midi
factory: container
polyphonic: false
tags: [container, serial, midi]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.midichain", type: companion, reason: "Enables MIDI events instead of blocking them" }
llmRef: |
  container.no_midi

  A serial container that blocks all MIDI events from reaching its children. Audio processing is identical to container.chain.

  Signal flow:
    audio in -> child[0] -> child[1] -> ... -> child[N] -> audio out
    MIDI events -> [blocked]

  CPU: negligible, monophonic

  Parameters:
    None

  When to use:
    Prevent oscillators from responding to MIDI pitch in polyphonic contexts (e.g. using an oscillator as an LFO). Block all MIDI-driven behaviour for a subtree of nodes.

  See also:
    [companion] container.midichain -- enables MIDI events instead of blocking them
---

The no_midi container processes children serially, identically to a regular chain, but blocks all MIDI events from reaching them. This is the inverse of [container.midichain]($SN.container.midichain$).

The most common use case is preventing an oscillator from responding to MIDI note pitch when it is being used as an LFO in a polyphonic synthesiser context. In such contexts, MIDI processing is enabled by default, and wrapping the oscillator in a no_midi container is the only way to disable it.

## Signal Path

::signal-path
---
glossary:
  functions:
    block MIDI:
      desc: "Silently drops all MIDI events before they reach children"
---

```
// container.no_midi - serial chain with MIDI blocking
// audio in -> audio out, MIDI blocked

dispatch(input) {
    block MIDI: all events dropped
    for each child in list order:
        child.process(input)    // in-place, no MIDI
}
```

::

All event types are blocked: note on/off, control change, pitch wheel, aftertouch, and any other MIDI message. There is no selective filtering -- audio processing is completely unaffected and children process audio normally.

**See also:** $SN.container.midichain$ -- enables MIDI events instead of blocking them
