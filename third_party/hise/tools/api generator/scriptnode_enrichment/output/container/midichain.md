---
title: MIDI Chain
description: "A serial container that enables sample-accurate MIDI event processing for its children."
factoryPath: container.midichain
factory: container
polyphonic: false
tags: [container, serial, midi]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: "MIDI event density", impact: "linear", note: "More events per block means more audio chunk boundaries" }
seeAlso:
  - { id: "container.no_midi", type: companion, reason: "Blocks MIDI events instead of enabling them" }
  - { id: "container.chain", type: disambiguation, reason: "Serial chain without MIDI event splitting" }
llmRef: |
  container.midichain

  A serial container that enables sample-accurate MIDI event processing. Splits the audio block at MIDI event timestamps so children process audio in chunks aligned to event boundaries.

  Signal flow:
    input audio block + MIDI events ->
      process(samples before event 1) ->
      handleEvent(event 1) ->
      process(samples before event 2) ->
      handleEvent(event 2) ->
      ... -> output

  CPU: low, monophonic
    Overhead proportional to MIDI event count per block.

  Parameters:
    None

  When to use:
    Required in effect plugin contexts to enable MIDI processing. Provides sample-accurate MIDI timing for nodes that respond to MIDI (oscillators, envelopes). Unnecessary in synthesiser or modulator contexts where MIDI is already enabled.

  See also:
    [companion] container.no_midi -- blocks MIDI events instead of enabling them
    [disambiguation] container.chain -- serial chain without MIDI event splitting
---

The MIDI chain processes children serially (like a regular chain) but additionally enables sample-accurate MIDI event processing. It splits the audio block at each MIDI event's timestamp, ensuring that events take effect at the exact sample position they occurred.

In effect plugin contexts, MIDI processing is disabled by default. Wrapping nodes in a midichain is the correct way to enable it. In synthesiser, envelope, or time-variant modulator contexts, MIDI processing is already enabled automatically, so midichain is unnecessary (though harmless).

## Signal Path

::signal-path
---
glossary:
  functions:
    split at event:
      desc: "Splits the audio block at each MIDI event timestamp for sample-accurate processing"
    serial dispatch:
      desc: "Processes children sequentially on each audio chunk"
---

```
// container.midichain - serial processing with MIDI event splitting
// audio + MIDI in -> audio out

dispatch(input, events) {
    for each event in timestamp order:
        serial dispatch: children.process(samples up to event)
        split at event: children.handleEvent(event)
    serial dispatch: children.process(remaining samples)
}
```

::

### Limitations

Midichain should not be nested inside frame-based or resampled containers, as both interfere with the timestamp-based audio splitting. With zero MIDI events in a block, the overhead is a single conditional check.

**See also:** $SN.container.no_midi$ -- blocks MIDI events instead of enabling them, $SN.container.chain$ -- serial chain without MIDI event splitting
