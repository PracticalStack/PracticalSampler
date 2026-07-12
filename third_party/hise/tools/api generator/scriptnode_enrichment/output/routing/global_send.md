---
title: Global Send
description: "Copies audio from the local signal chain into a shared buffer for cross-network routing."
factoryPath: routing.global_send
factory: routing
polyphonic: false
tags: [routing, audio, global, send]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.global_receive", type: companion, reason: "Receives the audio sent by this node" }
  - { id: "routing.send", type: alternative, reason: "Intra-network audio routing (compileable)" }
commonMistakes:
  - title: "Only one sender per signal slot"
    wrong: "Connecting two global_send nodes to the same signal slot"
    right: "Use one global_send per slot. Create additional slots if you need multiple senders."
    explanation: "Each signal slot accepts exactly one sender. A second global_send attempting to connect to an occupied slot is rejected with an error."
  - title: "Cannot be compiled to C++"
    wrong: "Using global_send in a network intended for C++ export"
    right: "Use routing.send and routing.receive for audio routing in compileable networks."
    explanation: "Global send and receive nodes depend on the IDE infrastructure and cannot be compiled. For compileable audio routing, use the intra-network send/receive pair instead."
llmRef: |
  routing.global_send

  Copies audio from the local signal chain into a shared buffer for cross-network routing. The local signal passes through unmodified -- the node does not consume or silence the audio. Cannot be compiled to C++.

  Signal flow:
    audio in -> copy to shared buffer (gain-scaled) -> audio out (passthrough)

  CPU: low, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 1.0). Linear gain applied to the copied signal.

  When to use:
    Cross-network audio routing where you need to send audio from one DspNetwork to another. Not commonly used in surveyed projects. For intra-network routing, prefer routing.send instead.

  Common mistakes:
    Only one sender per signal slot. Cannot be compiled to C++.

  See also:
    [companion] routing.global_receive -- receives the audio sent by this node
    [alternative] routing.send -- intra-network audio routing (compileable)
---

Copies audio from the local signal chain into a shared buffer that can be read by [global_receive]($SN.routing.global_receive$) nodes anywhere in HISE, including in other DspNetworks. The local signal passes through unmodified -- the node copies but does not consume the audio. The Value parameter applies a linear gain to the copied signal.

Each signal slot accepts exactly one sender. If a second global_send attempts to connect to an occupied slot, the connection is rejected. The send node overwrites the shared buffer on each processing block; if no global_receive is connected, the copied audio is discarded silently.

This node cannot be compiled to C++ -- for compileable audio routing within a single network, use [routing.send]($SN.routing.send$) instead.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Linear gain applied to the copied signal"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    copyToBuffer:
      desc: "Copies gain-scaled audio into the shared signal buffer"
---

```
// routing.global_send - copy audio to shared buffer
// audio in -> audio out (passthrough)

process(input) {
    copyToBuffer(input * Value)  // write to shared slot
    output = input               // signal passes through unchanged
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Linear gain multiplier applied to the audio copied into the shared buffer. Does not affect the passthrough signal.", range: "0.0 - 1.0", default: "1.0" }
---
::

**See also:** $SN.routing.global_receive$ -- receives the audio sent by this node, $SN.routing.send$ -- intra-network audio routing (compileable)
