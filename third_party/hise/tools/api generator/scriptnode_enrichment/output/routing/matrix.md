---
title: Matrix
description: "Applies an arbitrary channel routing matrix to the signal, configured through a visual editor."
factoryPath: routing.matrix
factory: routing
polyphonic: false
tags: [routing, matrix, channels, static]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: "channel count", impact: "linear", note: "Cost scales linearly with channel count; common stereo patterns are optimised" }
seeAlso:
  - { id: "routing.selector", type: alternative, reason: "Dynamic channel routing with modulatable parameters" }
  - { id: "RouteFX", type: module, reason: "Channel routing matrix" }
  - { id: "RoutingMatrix", type: api, reason: "RoutingMatrix API provides script-level channel routing control" }
commonMistakes:
  - title: "Expecting multichannel output in HISE standalone"
    wrong: "Testing multichannel routing in the HISE standalone app and expecting to see all output channels"
    right: "Compile the project as a plugin to verify multichannel output. The standalone app is limited to the physical audio interface outputs (typically 2)."
    explanation: "Multichannel routing only becomes visible when the project is compiled as a plugin. For development testing, add an inner container and treat it as the master output."
forumReferences:
  - { tid: 6347, reason: "container.multi vs routing.matrix for multichannel splitting" }
  - { tid: 12078, reason: "Stacking matrices for complex multi-output routing" }
llmRef: |
  routing.matrix

  Applies an arbitrary channel routing defined by a visual matrix editor. Each source channel maps to a destination channel; multiple sources can be summed into one destination. Channels mapped to no destination are muted. Supports one send channel (Shift-click) per matrix for parallel routing.

  Signal flow:
    multichannel in -> apply routing map -> multichannel out

  CPU: low, monophonic. Common stereo patterns (swap, mono sum) are optimised.

  Parameters:
    None. Routing is configured via the matrix editor (stored as EmbeddedData property).

  When to use:
    Used in 8 networks (rank 49). For static channel routing that does not need to change at runtime. Common uses: swapping left/right, summing to mono, routing multichannel signals. For dynamic routing, use routing.selector instead.

  Key details:
    Shift-click in the matrix editor creates a send (blue) connection that duplicates rather than moves.
    For splitting into stereo pairs, container.multi is simpler than a routing matrix.
    Stack multiple matrix nodes for complex fan-out routing.
    Multichannel output only visible in compiled plugins, not HISE standalone.

  See also:
    [alternative] routing.selector - dynamic channel routing with modulatable parameters
    [module] RouteFX - channel routing matrix in the module tree
    [api] RoutingMatrix - script-level channel routing control
---

Applies an arbitrary channel routing to the signal. The routing is configured through a visual matrix editor where you click cells to connect source channels to destination channels. Multiple source channels can be summed into one destination, and channels with no mapping are muted.

The node also supports send channels: hold Shift while clicking in the matrix editor to create a send connection (shown in blue), which duplicates the signal to the target channel pair rather than moving it. Each routing matrix supports one send connection; add another matrix node to get additional send connections.

Common stereo patterns -- swapping left and right, summing to mono, copying one channel to both -- are detected automatically and processed more efficiently. The routing is stored as an EmbeddedData property on the node and does not change at runtime. The matrix adapts to the channel count of the signal path -- it operates on however many channels are available, up to 16. The matrix has no user-facing parameters; all configuration is done through the visual editor in the node's property panel.

For dynamic channel routing that changes at runtime, use [routing.selector]($SN.routing.selector$) instead.

### When to Use container.multi Instead

For splitting a multichannel signal into separate stereo pairs for parallel processing in scriptnode, [container.multi]($SN.container.multi$) is often preferable -- no matrix wiring is required. The routing matrix is better suited to the module tree layer or for non-standard channel mappings.

### Complex Multi-Output Routing

The routing matrix handles one set of connections at a time and cannot do arbitrary fan-out in a single node. For complex routing scenarios (e.g. many samplers routed to different insert and send effects across many channel pairs), stack multiple routing matrix nodes or combine them with send containers.

## Signal Path

::signal-path
---
glossary:
  functions:
    applyRoutingMap:
      desc: "Routes each source channel to its mapped destination, summing where multiple sources share a destination"
---

```
// routing.matrix - static channel routing
// multichannel in -> multichannel out

process(channels) {
    temp = copy(channels)
    zero(channels)

    for each source in temp:
        dest = routingMap[source]
        if dest != muted:
            channels[dest] += temp[source]
}
```

::

**See also:** $SN.routing.selector$ -- dynamic channel routing with modulatable parameters, $MODULES.RouteFX$ -- channel routing matrix in the module tree, $API.RoutingMatrix$ -- script-level channel routing control
