---
title: Receive
description: "Receives audio from a connected send node and adds it to the current signal, scaled by the Feedback parameter."
factoryPath: routing.receive
factory: routing
polyphonic: true
tags: [routing, receive, aux, feedback]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "routing.send", type: companion, reason: "Sends the signal that this node receives" }
  - { id: "routing.global_cable", type: alternative, reason: "Receives control signals from independent networks" }
  - { id: "SendFX", type: module, reason: "Module-tree send effect for parallel signal routing" }
commonMistakes:
  - title: "Feedback defaults to zero"
    wrong: "Connecting a receive node and expecting audio to pass through immediately"
    right: "Set the Feedback parameter above 0 to hear the received signal."
    explanation: "Feedback defaults to 0.0, which completely mutes the received signal. This is intentional to prevent unexpected signal when first adding the node."
  - title: "Feedback is additive not crossfade"
    wrong: "Treating Feedback as a wet/dry mix between the input and received signal"
    right: "Feedback scales only the received signal, which is then added to the existing input: output = input + received * Feedback."
    explanation: "The input signal always passes through at full level. Feedback controls how much of the received signal is mixed on top. At Feedback = 1.0 the full received signal is added; the original is never attenuated."
  - title: "Receive placed before send in the graph"
    wrong: "Placing routing.receive before its paired routing.send (e.g. in the first branch of a split while the send is in the second)"
    right: "Reorder branches so the send branch executes first, or place both in the same chain with send above receive."
    explanation: "Scriptnode processes nodes top-down, left-to-right. If receive runs before send, the cable buffer has not been written yet, causing clicks, silence, or garbage at note on/off."
  - title: "Global receive prevents DLL compilation"
    wrong: "Using global receive nodes in a network that needs to be compiled"
    right: "Restructure routing to stay within a single network if DLL compilation is required."
    explanation: "Networks containing global receive nodes cannot be compiled to a native DLL. This is a hard limitation that affects the entire network."
forumReferences:
  - { tid: 1766, reason: "Send/receive ordering rules and colour-matched connection indicator" }
  - { tid: 4553, reason: "One-input-per-receive constraint and merging pattern" }
llmRef: |
  routing.receive

  Receives audio from a connected send node's cable buffer and adds it to the current signal, scaled by the Feedback parameter. Formula: output = input + received * Feedback. The input signal always passes through at full level.

  Signal flow:
    audio in + (cable buffer * Feedback) -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Feedback: 0.0 - 1.0 (default 0.0). Scales the received signal before adding to the input. NormalizedPercentage display.

  When to use:
    Very commonly used (rank 17, 21 instances). Always paired with routing.send. Two main patterns: AUX-style parallel effects and feedback delay loops.

  Key details:
    Each receive accepts exactly one input. Use multiple receives in a split to merge sources.
    Receive must come after send in graph order (top-down, left-to-right).
    Connected pairs are colour-matched in the graph editor.
    Oversample containers may break send/receive ordering.
    Global receive nodes prevent DLL compilation.

  Common mistakes:
    Feedback defaults to 0 -- must be increased to hear the routed signal.
    Feedback is additive, not a wet/dry crossfade.
    Receive before send causes clicks/garbage.
    Global receive prevents DLL compilation.

  See also:
    [companion] routing.send - sends the signal that this node receives
    [alternative] routing.global_cable - control signal routing across networks
    [module] SendFX - module-tree send effect for parallel signal routing
---

Receives audio from a connected [routing.send]($SN.routing.send$) node and adds it to the current signal. The received signal is scaled by the Feedback parameter before being mixed in: `output = input + received * Feedback`. The existing signal in the buffer always passes through at full level -- Feedback controls only the level of the received signal, not a wet/dry crossfade.

When no send node is connected, the receive passes its input through unchanged -- it does not output silence. Each voice reads from its own cable buffer in polyphonic contexts, maintaining voice isolation.

In a feedback loop configuration (receive placed before the processing, send placed after), there is one block of latency between the send and receive. This is inherent to the circular buffer design and cannot be eliminated.

### Connection Identification

When a receive node is properly connected to its send node, both nodes are tinted the same colour in the scriptnode graph as a visual indicator. If the colours do not match, the connection is broken -- typically because the send node's ID was changed after the receive was linked. Fix this by opening the send node's properties and reselecting the target from the dropdown.

### Merging Multiple Sources

Each receive node accepts exactly one input source. To receive from multiple send nodes at the same destination, add multiple receive nodes inside a [container.split]($SN.container.split$) -- each connected to a different send -- so their signals are summed at the split output.

### Oversample Container Interaction

Oversample containers create an oversampled copy of the signal internally, which may cause ordering problems between send and receive nodes placed inside or around them. Chain and multi containers handle the ordering correctly. If you encounter clicks or silence when combining oversample containers with send/receive routing, try restructuring so both nodes sit outside the oversample boundary.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Feedback:
      desc: "Scales the received signal before adding to the input"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    readFromCable:
      desc: "Reads audio data from the connected send node's cable buffer"
---

```
// routing.receive - additive signal receiver
// audio in + cable -> audio out

process(input) {
    received = readFromCable()
    output = input + received * Feedback
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Feedback, desc: "Scales the received signal before adding it to the input. At 0 the received signal is muted; at 1 the full signal is added.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.routing.send$ -- sends the signal that this node receives, $SN.routing.global_cable$ -- control signal routing across independent networks, $MODULES.SendFX$ -- module-tree send effect for parallel signal routing
