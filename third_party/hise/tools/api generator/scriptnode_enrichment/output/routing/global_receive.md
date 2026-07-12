---
title: Global Receive
description: "Receives audio from a global_send node and mixes it into the local signal chain."
factoryPath: routing.global_receive
factory: routing
polyphonic: true
tags: [routing, audio, global, receive]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "routing.global_send", type: companion, reason: "Sends the audio that this node receives" }
  - { id: "routing.receive", type: alternative, reason: "Intra-network audio routing (compileable)" }
commonMistakes:
  - title: "Received signal is added, not replaced"
    wrong: "Expecting the global_receive output to contain only the received signal"
    right: "The received audio is mixed on top of whatever signal is already flowing. Place the node after silence if you need the received signal alone."
    explanation: "Global receive uses additive mixing. If the local chain already carries audio, the received signal is summed into it rather than replacing it."
  - title: "Specs must match the sender"
    wrong: "Using different sample rates or channel counts between global_send and global_receive"
    right: "Ensure both nodes operate at the same sample rate and channel count. Block size at the receiver must not exceed the sender."
    explanation: "If sample rate, channel count, or block size do not match, the receiver silently produces no output. Check the node status for mismatch errors."
llmRef: |
  routing.global_receive

  Receives audio from a global_send node and adds it into the local signal chain. The received signal is mixed additively -- it does not replace existing audio. Cannot be compiled to C++.

  Signal flow:
    audio in + received buffer (gain-scaled) -> audio out

  CPU: low, polyphonic

  Parameters:
    Value: 0.0 - 1.0 (default 1.0). Linear gain applied to the received signal before mixing.

  When to use:
    Cross-network audio routing. Companion to global_send. Not commonly used in surveyed projects. For intra-network routing, prefer routing.receive instead.

  Common mistakes:
    Received signal is added, not replaced. Specs (sample rate, channels, block size) must match the sender.

  See also:
    [companion] routing.global_send -- sends the audio that this node receives
    [alternative] routing.receive -- intra-network audio routing (compileable)
---

Receives audio from a [global_send]($SN.routing.global_send$) node and mixes it into the local signal chain. The received signal is added on top of whatever audio is already flowing at the receive point -- it does not replace the existing signal. The Value parameter applies a linear gain to the received audio before mixing.

The receiver must match the sender's sample rate and channel count. The receiver's block size must not exceed the sender's block size. If any of these conditions are not met, no audio is received and the node reports an error status. This node cannot be compiled to C++ -- for compileable audio routing within a single network, use [routing.receive]($SN.routing.receive$) instead.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Linear gain applied to the received signal before mixing"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    readFromBuffer:
      desc: "Reads audio from the shared signal buffer and adds it to the local signal"
---

```
// routing.global_receive - mix received audio into local chain
// audio in + shared buffer -> audio out

process(input) {
    received = readFromBuffer() * Value
    output = input + received  // additive mix
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Linear gain multiplier applied to the received audio before it is mixed into the local signal.", range: "0.0 - 1.0", default: "1.0" }
---
::

### Multiple Receivers

Multiple global_receive nodes can read from the same signal slot independently. Each adds the received audio into its own local chain without affecting the others.

### Polyphonic Behaviour

In a polyphonic context, each voice maintains its own read offset into the shared buffer. This handles sub-block voice starts correctly, aligning each voice's read position with the monophonic shared buffer.

**See also:** $SN.routing.global_send$ -- sends the audio that this node receives, $SN.routing.receive$ -- intra-network audio routing (compileable)
