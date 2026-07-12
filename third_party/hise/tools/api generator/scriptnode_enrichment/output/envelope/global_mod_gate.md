---
title: Global Mod Gate
description: "Outputs a binary gate signal reflecting whether a global modulator's envelope is still active for the current voice."
factoryPath: envelope.global_mod_gate
factory: envelope
polyphonic: true
tags: [envelope, gate, global-modulator, voice-management]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "envelope.extra_mod_gate", type: disambiguation, reason: "Same behaviour but for extra modulation chains instead of global modulators" }
  - { id: "envelope.voice_manager", type: companion, reason: "Connect the gate output here to manage voice lifecycle" }
llmRef: |
  envelope.global_mod_gate

  Monitors a global modulator and outputs a binary gate: 1.0 while the modulator's envelope is active for the current voice, 0.0 when it has been released. Does not process audio.

  Signal flow:
    Control node - no audio processing
    GlobalModulatorContainer[Index] -> per-voice active check -> Gate output (0 or 1)

  CPU: negligible, polyphonic

  Parameters:
    Index (0-16, step 1, default 1; selects the global modulator slot to monitor)

  When to use:
    Voice lifecycle management driven by a global envelope modulator. Connect the gate output to envelope.voice_manager.

  See also:
    [disambiguation] envelope.extra_mod_gate -- same for extra modulation chains
    [companion] envelope.voice_manager -- voice lifecycle from gate output
---

Monitors a global modulator from the GlobalModulatorContainer and outputs a binary gate signal: 1.0 while the modulator's envelope is active for the current voice, 0.0 when the voice has been released. The node does not modify audio; it connects to the HISE module tree via a runtime target and reads the per-voice envelope state.

This is the gate companion to [core.global_mod]($SN.core.global_mod$), which provides the continuous envelope value. Use global_mod_gate when you need to know whether a voice is still active according to a global envelope, typically for connecting to an [envelope.voice_manager]($SN.envelope.voice_manager$) node.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Index:
      desc: "Selects which global modulator slot to monitor"
      range: "0 - 16"
      default: "1"
  functions:
    voiceActiveCheck:
      desc: "Checks whether the selected global modulator is still active for the current voice"
---

```
// envelope.global_mod_gate - global modulator voice gate
// control node, no audio processing

onNoteOn() {
    gateOut = 1.0                           // voice starts active
}

process() {
    if (!voiceActiveCheck(Index))
        gateOut = 0.0                       // voice released by global modulator
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: Index, desc: "Selects which modulator slot in the GlobalModulatorContainer to monitor. Uses the same indexing as core.global_mod.", range: "0 - 16", default: "1" }
---
::

### Setup

The node must be connected to a GlobalModulatorContainer via the runtime target system. Without a valid connection, the gate always reads as active.

### Limitations

For non-envelope modulators (such as voice-start or time-variant types), the gate always returns 1 because these modulator types do not have per-voice release states.

**See also:** $SN.envelope.extra_mod_gate$ -- same behaviour for extra modulation chains, $SN.envelope.voice_manager$ -- voice lifecycle management
