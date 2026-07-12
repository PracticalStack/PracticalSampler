---
title: Send Container
moduleId: SendContainer
type: SoundGenerator
subtype: SoundGenerator
tags: [container, routing]
builderPath: b.SoundGenerators.SendContainer
screenshot: /images/v2/reference/audio-modules/sendcontainer.png
cpuProfile:
  baseline: very low
  polyphonic: false
  scalingFactors: [effect chain complexity, number of internal channels]
seeAlso:
  - { id: SendFX, type: alternative, reason: "The send side of the send/return pair - routes audio into this container with adjustable gain and channel offset" }
forumReferences:
  - id: 1
    title: "SendContainer must follow its SendFX in the module tree"
    summary: "Placing a SendContainer before any SendFX that feeds it delays the send signal by one full audio buffer, breaking phase coherence."
    topic: 1223
  - id: 2
    title: "Gain and Balance parameters are not applied"
    summary: "The inherited Gain and Balance knobs on the SendContainer header have no effect on output level; control level via the FX chain or SendFX gain."
    topic: 9104
  - id: 3
    title: "Effect plugins require 'Enable Sound Generators FX' project setting"
    summary: "SendContainers only process audio in compiled effect plugins when the 'Enable Sound Generators FX' flag is enabled in project settings."
    topic: 6353
commonMistakes:
  - title: "Container Gain and Balance are unused"
    wrong: "Expecting the Gain or Balance knobs on the Send Container to affect the output level"
    right: "Control the output level using a SimpleGain effect in the container's FX chain, or adjust the Send Effect gain"
    explanation: "The Gain and Balance parameters are inherited from the base class but are not applied in the Send Container's render path. The output level is determined entirely by the Send Effect gain and the effects in the FX chain."
  - title: "Polyphonic effects forced to monophonic"
    wrong: "Adding polyphonic effects to the Send Container's FX chain and expecting per-voice processing"
    right: "All effects in the Send Container run monophonically"
    explanation: "The Send Container forces monophonic processing for all effects in its chain, regardless of whether the effect supports polyphonic mode."
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: low
  description: "A routing matrix and effect chain can replicate this pattern in scriptnode, though the send/return workflow is unique to the module tree"
llmRef: |
  Send Container (SoundGenerator)

  Receive end of a send/return routing pair. Accumulates audio from one or more Send Effect instances, processes it through an effect chain, and routes the result to the output via a routing matrix. Does not generate sound - acts as a summing bus with effects.

  Signal flow:
    Send Effects (external) -> internal buffer (additive sum) -> effect chain (MasterEffect only) -> routing matrix -> audio out -> clear buffer

  CPU: very low base, depends on FX chain contents. Monophonic (no per-voice processing).

  Parameters (inherited, mostly vestigial):
    Gain (0-100%, default 25%) - NOT applied in render path
    Balance (-1 to 1, default 0) - NOT applied in render path
    VoiceLimit (1-256, default 256) - irrelevant (single voice)
    KillFadeTime (0-20000 ms, default 20 ms) - irrelevant (single voice)

  Modulation chains (inherited, vestigial):
    Gain Modulation - NOT applied in render path
    Pitch Modulation - NOT applicable (no oscillator)

  FX chain:
    MasterEffect only (NoMidiInputConstrainer). All effects forced to monophonic. This is the primary user-facing feature.

  Routing:
    Resizable routing matrix maps internal channels to output channels. SendEffect channel offset determines which internal channel pair receives the send signal. Multiple SendEffects can target the same container (additive summing).

  When to use:
    Shared reverb, delay, or other effects across multiple sound generators. Classic aux send/return workflow. Place effects in the container's FX chain and use Send Effects on the source modules to control send levels.

  Common mistakes:
    Gain/Balance knobs have no effect - use FX chain or SendEffect gain instead.
    Polyphonic effects are forced monophonic in this container.

  See also:
    alternative SendFX - the send side of the pair
---

::category-tags
---
tags:
  - { name: container, desc: "Modules that hold and combine other sound generators" }
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Send Container screenshot](/images/v2/reference/audio-modules/sendcontainer.png)

The Send Container is the receive end of a send/return routing pair. It accumulates audio from one or more Send Effect instances into an internal buffer, processes the mixed signal through its effect chain, and routes the result to the output via a configurable routing matrix. It does not generate sound itself - it functions as a summing bus with an attached effect chain.

This is the standard way to share effects like reverb or delay across multiple sound generators in HISE. Place the desired effects in the container's FX chain, then add Send Effects to the source modules to control how much signal is routed to the container.

## Signal Path

::signal-path
---
glossary:
  functions:
    addSendSignal:
      desc: "Additively mixes a stereo signal from a Send Effect into the internal buffer at the specified channel offset and gain"
    routingMatrix:
      desc: "Maps each internal buffer channel to an output channel"
---

```
// Send Container - monophonic summing bus
// no voice processing, no sound generation

// Accumulation (called by each connected Send Effect)
for each SendEffect:
    internalBuffer[channelOffset] += sendSignal * sendGain

// Processing (per block)
effectChain(internalBuffer)

// Output routing
for each internalChannel:
    outputChannel = routingMatrix(internalChannel)
    output[outputChannel] += internalBuffer[internalChannel]

// Reset
clear(internalBuffer)
```

::

## Parameters

::parameter-table
---
groups:
  - label: Output (Inherited)
    params:
      - { name: Gain, desc: "Inherited from the base class. Not applied in the Send Container's render path. Has no effect on the output level.", range: "0 - 100%", default: "25%" }
      - { name: Balance, desc: "Inherited from the base class. Not applied in the Send Container's render path.", range: "-1 - 1", default: "0" }
  - label: Voice Management (Inherited)
    params:
      - { name: VoiceLimit, desc: "Inherited from the base class. The Send Container always operates with a single internal voice. This parameter has no effect.", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Inherited from the base class. Not applicable since the container has no voice lifecycle.", range: "0 - 20000 ms", default: "20 ms" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Gain Modulation", desc: "Inherited from the base class. Not applied in the Send Container's render path.", scope: "monophonic", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Inherited from the base class. Not applicable - the Send Container has no oscillator or pitch concept.", scope: "monophonic", constrainer: "Any" }
---
::

### Module Tree Placement

The Send Container must be placed **after** all Send Effects that route to it in the module tree. If it appears before a Send Effect, the send signal is delayed by one full audio buffer, breaking phase coherence between dry and wet paths. [1]($FORUM_REF.1223$)

### FX Chain

The FX chain accepts MasterEffect-type effects only. Polyphonic effects placed in the chain are forced to process monophonically. The inherited Gain and Balance parameters in the header are not applied in the render path - control the output level by placing a $MODULES.SimpleGain$ in the FX chain. [2]($FORUM_REF.9104$)

### Multichannel Routing

Multiple Send Effects can target the same container, with signals summed additively. The routing matrix supports more than two internal channels. Each Send Effect's channel offset targets a specific stereo pair within the internal buffer, enabling multiple parallel send busses in a single container. [3]($FORUM_REF.13113$)

### FX Plugin Export

In compiled effect plugins, the Send Container only processes audio if the 'Enable Sound Generators FX' flag is enabled in the project settings. [4]($FORUM_REF.6353$)

**See also:** $MODULES.SendFX$ -- the send side of the send/return pair, routes audio into this container with adjustable gain and channel offset