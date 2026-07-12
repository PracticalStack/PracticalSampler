---
title: Send Effect
moduleId: SendFX
type: Effect
subtype: MasterEffect
tags: [routing, mixing]
builderPath: b.Effects.SendFX
screenshot: /images/v2/reference/audio-modules/sendfx.png
cpuProfile:
  baseline: very low
  polyphonic: false
  scalingFactors: [smoothing]
seeAlso:
  - { id: SendContainer, type: alternative, reason: "The receive side of the send/return pair - accumulates signals from Send Effects and processes them through its FX chain" }
  - { id: "routing.send", type: scriptnode, reason: "Scriptnode send node for bus-style routing in DspNetwork" }
  - { id: "routing.receive", type: scriptnode, reason: "Scriptnode receive node for bus-style routing in DspNetwork" }
commonMistakes:
  - title: "Send copies signal, doesn't remove it"
    wrong: "Expecting the Send Effect to remove the signal from the original chain"
    right: "The Send Effect copies the signal to the target container while the original passes through unmodified"
    explanation: "This is a send, not an insert. The dry signal continues through the effect chain as if the Send Effect were not there. Only a copy is routed to the Send Container."
  - title: "Default Gain is effectively silent"
    wrong: "Adding a Send Effect but leaving Gain at the default and wondering why nothing happens"
    right: "Raise the Gain from -100 dB to the desired send level"
    explanation: "The default gain is -100 dB (effectively silent). You must increase it before any signal reaches the Send Container."
  - title: "Smoothing prevents clicks on automation"
    wrong: "Disabling Smoothing and automating the Gain parameter during playback"
    right: "Keep Smoothing enabled when automating Gain to avoid clicks"
    explanation: "With Smoothing off, gain changes take effect immediately with no crossfade, which produces audible clicks on sudden value changes."
forumReferences:
  - id: 1
    title: "SendFX in Master Container causes instant feedback"
    summary: "A SendFX placed directly in the Master Container feeds its output back into itself, producing an immediate full-volume feedback loop."
    topic: 5032
  - id: 2
    title: "SendFX send is post-fader only"
    summary: "The send is tapped after the sound generator's own gain stage; there is no pre-fader option in the built-in module."
    topic: 76
  - id: 3
    title: "SendIndex can be scripted to switch containers at runtime"
    summary: "The SendIndex parameter (attribute index 2) can be changed via setAttribute() to dynamically redirect the send to a different container."
    topic: 13113
customEquivalent:
  approach: scriptnode
  moduleType: Effect
  complexity: low
  description: "A gain node feeding a global send cable can replicate this pattern in scriptnode"
llmRef: |
  Send Effect (MasterEffect)

  Routes a copy of the input signal to a Send Container at an adjustable gain level. The original audio passes through unmodified - this is a pure send, not an insert. Designed for aux send/return workflows where multiple sources share a common effect chain.

  Signal flow:
    audio in -> gain calculation (dB to linear, optional 80ms smoothing, modulation, bypass ramp) -> additive mix into Send Container buffer -> audio out (passthrough, unchanged)

  CPU: very low. Monophonic (MasterEffect).

  Parameters:
    Gain (-100 to 0 dB, default -100 dB) - send level in decibels. Default is silent.
    ChannelOffset (0-16, default 0) - stereo pair offset within the Send Container's internal buffer
    SendIndex (0-128, default 0) - which Send Container to target. 1-based: 0 = disconnected, 1 = first container.
    Smoothing (Off/On, default On) - enables 80ms block-rate gain smoothing to prevent clicks

  Modulation chains:
    Send Modulation - scales the send gain per block (ScaleOnly mode). Sampled at block start and end for a ramped gain envelope.

  When to use:
    Routing audio to a shared reverb, delay, or other effect hosted in a Send Container. Place a Send Effect on each source module and control how much signal reaches the shared effect.

  Common mistakes:
    Default gain is -100 dB (silent) - must be raised to hear any send.
    This is a send, not an insert - the dry signal is untouched.
    Disabling Smoothing causes clicks on gain automation.

  Custom equivalent:
    scriptnode Effect with gain node and global send cable.

  See also:
    alternative SendContainer - the receive side of the pair
    scriptnode routing.send + routing.receive - send/receive pair for bus-style routing in DspNetwork
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
  - { name: mixing, desc: "Effects that control volume, stereo width, or stereo balance" }
---
::

![Send Effect screenshot](/images/v2/reference/audio-modules/sendfx.png)

The Send Effect routes a copy of the input signal to a Send Container at an adjustable gain level. The original audio passes through completely unmodified - this is a pure send, not an insert or diversion. It is the standard way to feed audio into a shared effect chain hosted by a Send Container.

Each Send Effect targets a single Send Container selected by index. The send level is controlled in decibels with optional smoothing to prevent clicks during automation. A modulation chain allows dynamic control of the send level from envelopes, LFOs, or other modulators.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gain:
      desc: "Send level in decibels, converted to linear gain"
      range: "-100 - 0 dB"
      default: "-100 dB"
    ChannelOffset:
      desc: "Stereo pair offset within the Send Container's internal buffer"
      range: "0 - 16"
      default: "0"
    SendIndex:
      desc: "Which Send Container to target (1-based, 0 = disconnected)"
      range: "0 - 128"
      default: "0"
    Smoothing:
      desc: "Enables 80ms block-rate gain smoothing"
      range: "Off / On"
      default: "On"
  functions:
    addSendSignal:
      desc: "Additively mixes the gained signal into the target Send Container's internal buffer at the specified channel offset"
  modulations:
    SendModulation:
      desc: "Scales the send gain per block"
      scope: "monophonic"
---

```
// Send Effect - monophonic, pure send
// stereo in -> stereo out (passthrough)

// Gain calculation (per block)
startGain = dBToLinear(Gain) * SendModulation[blockStart]
endGain   = dBToLinear(Gain) * SendModulation[blockEnd]

// Bypass ramp (on state change)
if bypassing:  endGain = 0
if resuming:   startGain = 0

// Smoothing
if Smoothing:
    startGain = smoothedValue.current    // 80ms ramp
    endGain   = smoothedValue.next
else:
    startGain = targetGain
    endGain   = targetGain

// Send to container (does not modify input)
if SendIndex > 0:
    addSendSignal(input, startGain -> endGain, ChannelOffset)

// Output is the original input, unchanged
output = input
```

::

## Parameters

::parameter-table
---
groups:
  - label: Send Level
    params:
      - { name: Gain, desc: "Send level in decibels. Converted to linear gain before applying. The default is -100 dB, which is effectively silent - you must raise this to hear any send signal.", range: "-100 - 0 dB", default: "-100 dB" }
      - { name: Smoothing, desc: "Enables block-rate gain smoothing with an 80ms ramp time. Prevents clicks when the gain changes. When off, gain changes take effect immediately.", range: "Off / On", default: "On" }
  - label: Send Routing
    params:
      - { name: SendIndex, desc: "Selects which Send Container receives the signal. Uses 1-based indexing: 0 means disconnected, 1 targets the first Send Container, 2 the second, and so on. The editor shows container names in a dropdown.", range: "0 - 128", default: "0" }
      - { name: ChannelOffset, desc: "Stereo pair offset within the target Send Container's internal buffer. An offset of 0 writes to channels 1-2, an offset of 2 writes to channels 3-4, and so on. Used for multichannel routing setups.", range: "0 - 16", default: "0" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Send Modulation", desc: "Scales the send gain. Sampled at the start and end of each block to create a per-block gain ramp. Operates in scale-only mode: the modulation output multiplies the parameter gain rather than replacing it.", scope: "monophonic", constrainer: "Any" }
---
::

### Placement

Do not place a Send Effect in the Master Container - it will feed its output back into itself, producing an immediate feedback loop. Always place Send Effects inside child containers. [1]($FORUM_REF.5032$)

The send is post-fader relative to the sound generator's gain stage. There is no pre-fader option. [2]($FORUM_REF.76$)

### Defaults and Bypass

The default gain of -100 dB means a newly added Send Effect is silent. Bypass uses a soft ramp: send gain ramps to zero over one block when engaged and back to the target when disengaged, regardless of the Smoothing toggle.

### Scripting

The `SendIndex` parameter (attribute index 2) can be set via `setAttribute()` to dynamically switch the target Send Container at runtime. Use `Engine.getDecibelsForGainFactor()` when exposing SendLevel as a dB knob. [3]($FORUM_REF.13113$)

**See also:** $MODULES.SendContainer$ -- the receive side of the send/return pair, accumulates signals from Send Effects and processes them through its FX chain, $SN.routing.send$ + $SN.routing.receive$ -- scriptnode send/receive pair for bus-style routing in DspNetwork