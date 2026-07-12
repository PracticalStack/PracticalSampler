---
title: Global Cable
description: "Routes a normalised control value across all DspNetworks via a named cable connection."
factoryPath: routing.global_cable
factory: routing
polyphonic: false
tags: [routing, control, global, cable]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.local_cable", type: disambiguation, reason: "Network-scoped alternative for connections within a single network" }
  - { id: "GlobalCable", type: api, reason: "Direct equivalent -- scriptnode node wraps the GlobalCable API for cross-network value distribution" }
commonMistakes:
  - title: "Values clamped to 0..1 range"
    wrong: "Sending a value of 440.0 through a global_cable expecting it to arrive unchanged"
    right: "All values are clamped to 0..1. Use a range converter on the receiving end to map back to the target range."
    explanation: "The global cable system clamps all values to 0..1 internally. To transmit values outside this range, normalise before sending and denormalise after receiving."
llmRef: |
  routing.global_cable

  Routes a normalised 0..1 control value across all DspNetworks via a named cable connection. Bidirectional -- any global_cable node on the same Connection ID can send or receive. A recursion guard prevents feedback loops.

  Signal flow:
    Control node -- no audio processing
    Value param -> broadcast to all peers on same Connection -> modulation output on receivers

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 1.0). Control value sent/received through the cable.

  When to use:
    Cross-network control value routing. 13 instances across surveyed projects (rank 30). Use for linking parameters between different DspNetworks or for OSC integration (IDs starting with /). Prefer routing.local_cable when connections stay within a single network.

  Common mistakes:
    Values are always clamped to 0..1 -- use range converters for wider ranges.

  See also:
    [disambiguation] routing.local_cable -- network-scoped cable for single-network use
    [api] GlobalCable -- HiseScript API for creating and connecting global cables
---

Routes a normalised control value between any number of nodes across all DspNetworks via a shared named connection. Each global_cable node acts as both sender and receiver -- setting the Value parameter broadcasts it to every other global_cable sharing the same Connection ID, and incoming values from peers appear on the modulation output. A built-in recursion guard prevents feedback loops.

Cable IDs starting with `/` integrate with OSC. When an OSC receiver is active, incoming OSC messages are normalised to 0..1 using the configured input range. When an OSC sender is active, outgoing values are denormalised using the configured output range.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Control value sent through the cable"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    broadcastToPeers:
      desc: "Sends the value to all other global_cable nodes on the same Connection"
---

```
// routing.global_cable - bidirectional control value router
// control in -> broadcast -> modulation out (on peers)

onValueChange(Value) {
    broadcastToPeers(Value)  // all peers on same Connection
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
      - { name: Value, desc: "Control value to send or receive. Clamped to 0..1 by the cable system.", range: "0.0 - 1.0", default: "1.0" }
---
::

### Compilation

Global cables can be compiled to C++ for use in exported plugins. The compiled node uses hash-based addressing to connect to the cable system at runtime. External nodes can send values back to HISE through the cable system -- useful for displaying internal state such as gain reduction or level metering on the UI.

**See also:** $SN.routing.local_cable$ -- network-scoped cable for connections within a single network, $API.GlobalCable$ -- HiseScript API for creating and connecting global cables
