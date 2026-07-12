---
title: Offline
description: "A container for offline processing that skips the realtime audio callback."
factoryPath: container.offline
factory: container
polyphonic: false
tags: [container, serial, offline]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
llmRef: |
  container.offline

  A container for offline (non-realtime) processing. Children are prepared and reset normally but never receive process or handleHiseEvent calls during the realtime audio callback. Audio passes through unmodified.

  Signal flow:
    input -> [passthrough] -> output
    Children: prepared but not processed in realtime

  CPU: negligible (no-op during realtime), monophonic

  Parameters:
    None

  When to use:
    Offline rendering tasks such as sample analysis, file processing, or convolution IR generation. Children can be invoked programmatically outside the realtime audio callback.

  See also:
    (none)
---

The offline container prepares its children normally but skips all realtime audio processing. During the audio callback, `process`, `processFrame`, and `handleHiseEvent` are all no-ops - audio passes through unmodified.

Children inside the offline container have valid preparation state (sample rate, block size, channel count) and can be invoked programmatically outside the realtime callback for offline rendering tasks such as sample analysis, file processing, or convolution IR generation.

## Signal Path

::signal-path
---
glossary:
  functions:
    passthrough:
      desc: "Audio passes through without any processing"
---

```
// container.offline - no realtime processing
// audio in -> audio out (unmodified)

dispatch(input) {
    passthrough: input unchanged
    // children are prepared but never called during realtime
}
```

::

