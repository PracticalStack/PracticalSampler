---
title: Extra Mod Gate
description: "Outputs a binary gate signal reflecting whether an extra modulation chain's envelope is still active for the current voice."
factoryPath: envelope.extra_mod_gate
factory: envelope
polyphonic: true
tags: [envelope, gate, extra-modulation, voice-management]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "envelope.global_mod_gate", type: disambiguation, reason: "Same behaviour but for global modulators instead of extra modulation chains" }
  - { id: "envelope.voice_manager", type: companion, reason: "Connect the gate output here to manage voice lifecycle" }
llmRef: |
  envelope.extra_mod_gate

  Monitors an extra modulation chain and outputs a binary gate: 1.0 while the chain's envelope is active for the current voice, 0.0 when it has been released. Does not process audio.

  Signal flow:
    Control node - no audio processing
    ExtraModulationChain[Index] -> per-voice active check -> Gate output (0 or 1)

  CPU: negligible, polyphonic

  Parameters:
    Index (0-16, step 1, default 1; selects the extra modulation chain to monitor)

  When to use:
    Voice lifecycle management driven by an extra modulation chain envelope. Connect the gate output to envelope.voice_manager.

  See also:
    [disambiguation] envelope.global_mod_gate -- same for global modulators
    [companion] envelope.voice_manager -- voice lifecycle from gate output
---

Monitors an extra modulation chain and outputs a binary gate signal: 1.0 while the chain's envelope is active for the current voice, 0.0 when the voice has been released. The node does not modify audio; it connects to the extra modulation chain via a runtime target and reads the per-voice envelope state.

This is the gate companion to [core.extra_mod]($SN.core.extra_mod$), which provides the continuous modulation value. Use extra_mod_gate when you need to know whether a voice is still active according to an extra modulation chain's envelope, typically for connecting to an [envelope.voice_manager]($SN.envelope.voice_manager$) node. It works identically to [envelope.global_mod_gate]($SN.envelope.global_mod_gate$) but targets extra modulation chains instead of the GlobalModulatorContainer.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Index:
      desc: "Selects which extra modulation chain to monitor"
      range: "0 - 16"
      default: "1"
  functions:
    voiceActiveCheck:
      desc: "Checks whether the selected extra modulation chain is still active for the current voice"
---

```
// envelope.extra_mod_gate - extra modulation chain voice gate
// control node, no audio processing

onNoteOn() {
    gateOut = 1.0                           // voice starts active
}

process() {
    if (!voiceActiveCheck(Index))
        gateOut = 0.0                       // voice released by extra modulation chain
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: Index, desc: "Selects which extra modulation chain to monitor. Uses the same indexing as core.extra_mod.", range: "0 - 16", default: "1" }
---
::

### Setup

The node must be connected to an extra modulation chain via the runtime target system. Without a valid connection, the gate always reads as active.

### Limitations

For non-envelope modulators, the gate always returns 1 because these modulator types do not have per-voice release states.

**See also:** $SN.envelope.global_mod_gate$ -- same behaviour for global modulators, $SN.envelope.voice_manager$ -- voice lifecycle management
