---
title: Routing Matrix
moduleId: RouteFX
type: Effect
subtype: MasterEffect
tags: [routing]
builderPath: b.Effects.RouteFX
screenshot: /images/v2/reference/audio-modules/routefx.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: SendFX, type: alternative, reason: "Routes signal to a separate Send Container with gain control, rather than duplicating within the same buffer" }
  - { id: "routing.matrix", type: scriptnode, reason: "Scriptnode channel routing matrix for DspNetwork signal paths" }
commonMistakes:
  - title: "Routing matrix copies, not moves"
    wrong: "Expecting the Routing Matrix to move signal from one channel to another"
    right: "The Routing Matrix copies signal additively - the source channel keeps its signal"
    explanation: "Each send route adds the source channel's signal to the target channel. The source is never zeroed. To move signal, you would need to mute the source channel separately."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: simple
  description: "A scriptnode network with routing nodes can replicate channel send/duplicate behaviour"
llmRef: |
  Routing Matrix (MasterEffect)

  A pure routing utility with no parameters and no modulation chains. Duplicates and distributes audio across channels using an additive send matrix. The source signal is preserved on its original channel while a copy is added to the target channel.

  Signal flow:
    multichannel in -> for each send route: target += source -> multichannel out

  CPU: negligible. Monophonic.

  Parameters: none.

  When to use:
    Building multichannel aux-style signal paths, duplicating channels for parallel processing, or distributing a stereo signal across a wider channel layout. For gain-controlled sends to a separate effect chain, use Send Effect instead.

  Common mistakes:
    This is additive copy, not move - source channels retain their signal.

  Custom equivalent:
    scriptnode HardcodedFX with routing nodes.

  See also:
    alternative SendFX - gain-controlled send to a separate container
    scriptnode routing.matrix - channel routing matrix in DspNetwork
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Routing Matrix screenshot](/images/v2/reference/audio-modules/routefx.png)

The Routing Matrix is a pure routing utility that duplicates and distributes audio across channels. It has no parameters and no modulation chains - its entire behaviour is defined by the routing matrix configured in its editor panel.

Each send route additively copies a source channel's signal to a target channel. The source channel retains its original signal. Multiple sources can target the same destination, in which case their signals are summed. The module supports up to 16 channels.

## Signal Path

::signal-path
---
glossary:
  functions:
    addSend:
      desc: "Additively copies the source channel's signal to the target channel"
---

```
// Routing Matrix - multichannel routing utility
// multichannel in -> multichannel out

for each channel:
    target = routingMatrix.getSendTarget(channel)
    if target exists:
        addSend(output[target], input[channel])

// Source channels are unchanged - this is additive copy, not move
```

::

### Limitations

The Routing Matrix cannot be soft-bypassed - it is always active because downstream modules may depend on the routing. All routing is configured through the editor panel's matrix grid with no scriptable parameters for runtime control. For gain-controlled routing to a separate effect chain (aux send/return), use a $MODULES.SendFX$ targeting a $MODULES.SendContainer$ instead.

**See also:** $MODULES.SendFX$ -- Gain-controlled send to a separate Send Container, for aux send/return workflows, $SN.routing.matrix$ -- Scriptnode channel routing matrix for DspNetwork signal paths
