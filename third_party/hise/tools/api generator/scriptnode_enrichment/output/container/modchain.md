---
title: Modulation Chain
description: "A serial container that processes children at control rate without affecting the parent audio signal."
factoryPath: container.modchain
factory: container
polyphonic: false
tags: [container, serial, modulation, control-rate]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: "HISE_EVENT_RASTER", impact: "divisor", note: "Instrument plugins process at 1/8 rate; effect plugins at full rate" }
seeAlso:
  - { id: "container.chain", type: disambiguation, reason: "Audio-rate serial chain that modifies the signal" }
commonMistakes:
  - title: "Expecting modchain to modify audio"
    wrong: "Placing audio effect nodes inside a modchain and expecting the parent signal to change"
    right: "Use a regular container.chain for audio processing. Place modulation source nodes (core.peak, envelopes) inside modchain."
    explanation: "Modchain does not modify the parent audio signal. It processes a separate internal control buffer at reduced rate. Nodes inside it should generate modulation output via cables, not process audio."
  - title: "Orphan modulation connections after deleting nodes"
    wrong: "Deleting a modulating node and assuming the connection is removed automatically"
    right: "After deleting a modulation source node, manually inspect the target node and remove any remaining connections."
    explanation: "When a node that modulates another node is deleted, the connection can persist on the target node, preventing it from working normally. The symptom is a parameter that no longer responds as expected."
  - title: "Setting parameter range on source only"
    wrong: "Defining the min/max range on the modulation source node but not on the target node"
    right: "Define the parameter range (min/max) on both the source and the target node for connections to work."
    explanation: "Modulation connections require ranges defined on both ends. Use Shift-click on the min/max fields in the range editor to set values. Setting min greater than max inverts the modulation curve."
forumReferences:
  - { tid: 4978, reason: "Modchain signal flow and connection range requirements" }
llmRef: |
  container.modchain

  A serial container that processes children at control rate on an internal mono buffer without affecting the parent audio signal. Used to build modulation sources.

  Signal flow:
    parent audio -> [passthrough, unmodified] -> parent output
    internal: mono control buffer (zeroed) -> children process at reduced rate

  CPU: low, monophonic
    Instrument plugins: children run at 1/8 sample rate (HISE_EVENT_RASTER = 8).
    Effect plugins: children run at full sample rate (HISE_EVENT_RASTER = 1).

  Parameters:
    None

  When to use:
    Building modulation sources from DSP nodes. Typically contains oscillators, math nodes, and a core.peak node at the end to output the modulation signal via a cable.

  Common mistakes:
    Modchain does not modify parent audio. Use container.chain for audio processing.
    Deleting a modulation source can leave orphan connections on the target. Inspect and remove them manually.
    Parameter ranges must be defined on both source AND target for connections to work.

  Key details:
    To combine multiple mod signals, use a split inside the modchain with a core.peak after it.
    Avoid overusing send/receive inside modchains -- prefer the normal top-down signal flow.

  See also:
    [disambiguation] container.chain -- audio-rate serial chain that modifies the signal
---

The modulation chain processes its children on a separate internal mono buffer at a reduced sample rate, without affecting the parent audio signal. This is the standard way to build modulation sources from DSP nodes in scriptnode.

In instrument plugin contexts, children run at 1/8 of the original sample rate and block size (controlled by the `HISE_EVENT_RASTER` preprocessor, defaulting to 8). This significantly reduces CPU cost for modulation signals that do not require audio-rate precision. In effect plugin contexts, `HISE_EVENT_RASTER` is 1, so children run at full rate - only the mono channel restriction applies.

A typical modchain contains oscillator or math nodes to shape a control signal, with a [core.peak]($SN.core.peak$) node as the last child to output the result as a modulation signal via a cable. For a consistent modulation update rate independent of the host buffer size, wrap both the modchain and its modulation targets inside a fixed block size container.

## Signal Path

::signal-path
---
glossary:
  functions:
    control-rate dispatch:
      desc: "Processes children on an internal mono buffer at reduced sample rate"
---

```
// container.modchain - control-rate modulation chain
// parent audio passes through unmodified

dispatch(parentAudio) {
    // parent audio is not touched
    controlBuffer = zeroed mono buffer
    control-rate dispatch: children.process(controlBuffer)
    // modulation output sent via cables from child nodes
}
```

::

The modchain itself is not a modulation source -- nodes inside it (such as [core.peak]($SN.core.peak$)) produce the actual modulation output. When nested inside a frame-based container, the control-rate downsampling is disabled and children run at full audio rate. Modchain should not be nested inside a resampled container.

### Combining Multiple Signals

To combine multiple modulation signals into a single output, place a [container.split]($SN.container.split$) inside the modchain with each signal as a child, then add a [core.peak]($SN.core.peak$) after the split to capture the summed result. Avoid using many send/receive nodes inside a modchain for summing or routing -- the normal top-down signal flow is simpler and more efficient. Only use send/receive when genuinely needed (e.g. for feedback loops).

**See also:** $SN.container.chain$ -- audio-rate serial chain that modifies the signal
