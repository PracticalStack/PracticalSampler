---
title: Local Cable
description: "Routes a normalised control value between nodes sharing the same ID within a single network."
factoryPath: routing.local_cable
factory: routing
polyphonic: false
tags: [routing, control, local, cable]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.local_cable_unscaled", type: disambiguation, reason: "Same behaviour but without value normalisation" }
  - { id: "routing.global_cable", type: disambiguation, reason: "Cross-network cable for routing values between different DspNetworks" }
commonMistakes:
  - title: "Scope limited to single network"
    wrong: "Expecting a local_cable to communicate with nodes in a different DspNetwork"
    right: "Use routing.global_cable for cross-network value routing. Local cables are scoped to the network they belong to."
    explanation: "Each DspNetwork has its own cable manager. Local cables cannot reach across network boundaries. Use global_cable for cross-network communication."
llmRef: |
  routing.local_cable

  Routes a normalised 0..1 control value between nodes sharing the same LocalId within a single DspNetwork. Bidirectional with recursion guard. Compileable to C++.

  Signal flow:
    Control node -- no audio processing
    Value param -> broadcast to peers with same LocalId -> modulation output on receivers

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Control value routed through the cable.

  When to use:
    Replacing visual cable connections within a network to reduce clutter. Newer feature not yet widely adopted in surveyed projects. Prefer over global_cable when connections stay within a single network.

  See also:
    [disambiguation] routing.local_cable_unscaled -- same behaviour without value normalisation
    [disambiguation] routing.global_cable -- cross-network cable
---

Routes a normalised control value between nodes sharing the same LocalId within a single DspNetwork. Like [global_cable]($SN.routing.global_cable$), local_cable is bidirectional -- setting the Value parameter broadcasts to all peers, and incoming values from peers appear on the modulation output. A recursion guard prevents feedback loops.

The primary use case is reducing visual clutter in complex networks. Instead of drawing long cable connections across the graph, you can place local_cable nodes near the source and target and assign them the same ID. When compiling to C++, local cables are automatically replaced with direct parameter connections.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Control value routed through the cable"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    broadcastToPeers:
      desc: "Sends the value to all local_cable nodes with the same LocalId"
---

```
// routing.local_cable - network-scoped control value router
// control in -> broadcast -> modulation out (on peers)

onValueChange(Value) {
    broadcastToPeers(Value)  // all peers with same LocalId
    // each peer forwards to its modulation output
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Normalised control value. Setting this broadcasts to all peers with the same LocalId and sends to the modulation output.", range: "0.0 - 1.0", default: "0.0" }
---
::

### Cable Management

The network provides UX helpers for working with local cables. A panel lists all active cable IDs with LED indicators showing current values. Clicking an ID highlights all nodes using that cable. Right-clicking offers an option to replace the local cable with a direct connection (and vice versa from the connection editor).

### Limitations

Up to 64 unique cable IDs can be used per network.

**See also:** $SN.routing.local_cable_unscaled$ -- same behaviour without value normalisation, $SN.routing.global_cable$ -- cross-network cable for routing values between different DspNetworks
