---
title: PPQ
description: "Sends a normalised modulation signal representing the host playback position within a musical time window when transport starts or the position jumps."
factoryPath: control.ppq
factory: control
polyphonic: true
tags: [control, transport, tempo-sync, ppq]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.transport", type: companion, reason: "Provides a simple on/off signal for transport state" }
  - { id: "core.clock_ramp", type: alternative, reason: "Provides a continuously updating tempo-synced ramp" }
commonMistakes:
  - title: "Does not update continuously during playback"
    wrong: "Expecting the output to ramp smoothly as the transport plays"
    right: "The value only updates on transport start and position jumps (e.g. loop boundaries, seeks). For a continuous ramp, use core.clock_ramp."
    explanation: "The node captures a snapshot of the PPQ position at transport events. It does not track the position in real time during ongoing playback."
llmRef: |
  control.ppq

  Sends a normalised 0..1 modulation signal derived from the host playback position when transport starts or the position jumps.

  Signal flow:
    Transport event -> capture PPQ position -> wrap into loop range (Tempo * Multiplier) -> normalise to 0..1 -> modulation output

  CPU: negligible, polyphonic

  Parameters:
    Tempo: tempo sync values (default 1/4, index 5). Defines the base loop length in musical time.
    Multiplier: 1 - 16 (default 1). Multiplies the Tempo value for longer time windows.

  When to use:
    Position-dependent modulation that responds to DAW playback location -- for example, cycling an effect parameter across a multi-bar phrase. Not observed in the surveyed projects. For continuous tempo-synced modulation, use core.clock_ramp instead.

  Common mistakes:
    Does not update continuously. Only fires on transport start and position jumps.

  See also:
    [companion] control.transport -- simple on/off transport state
    [alternative] core.clock_ramp -- continuous tempo-synced ramp
---

Captures the host playback position when the transport starts or the position jumps (loop boundaries, user seeks) and outputs a normalised 0..1 value representing where that position falls within a musical time window. The window length is defined by Tempo multiplied by Multiplier.

The output is calculated as `fmod(ppqPosition, loopLength) / loopLength`, where `loopLength` is derived from the Tempo and Multiplier parameters. For example, with Tempo set to 1/4 and Multiplier set to 2, the window spans a half bar. A playback position at the third quarter note of a bar would output 0.5.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Tempo:
      desc: "Base musical time value defining the loop window"
      range: "Tempo sync values"
      default: "1/4"
    Multiplier:
      desc: "Multiplier applied to the Tempo value for longer windows"
      range: "1 - 16"
      default: "1"
  functions:
    wrapAndNormalise:
      desc: "Wraps the PPQ position into the loop range and normalises to 0..1"
---

```
// control.ppq - playback position to normalised modulation
// transport event -> normalised position out (0..1)

onTransportStart(ppqPosition) {
    loopLength = tempoToQuarters(Tempo) * Multiplier
    output = wrapAndNormalise(ppqPosition, loopLength)
    sendToOutput(output)
}

onPositionJump(ppqPosition) {
    loopLength = tempoToQuarters(Tempo) * Multiplier
    output = wrapAndNormalise(ppqPosition, loopLength)
    sendToOutput(output)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: Tempo, desc: "The base musical time value that defines the loop window. Standard tempo sync values from whole note to 1/64 triplet.", range: "Tempo sync values", default: "1/4" }
      - { name: Multiplier, desc: "Multiplies the Tempo value to create longer time windows. A Tempo of 1/4 with a Multiplier of 4 spans one full bar.", range: "1 - 16", default: "1" }
---
::

The value only updates on two events: transport start (when playing begins) and position jumps (loop boundaries or user seeks). It does not update continuously during playback. If the user moves the playback ruler while the transport is stopped, the new position is picked up the next time playback starts.

> [!Tip:Debugging with cable_expr] Connect a [control.cable_expr]($SN.control.cable_expr$) node with debug mode enabled to visualise the output. Use the HISE Controller popup to simulate DAW transport positions and observe how the ppq node responds.

**See also:** $SN.control.transport$ -- simple on/off transport state, $SN.core.clock_ramp$ -- continuous tempo-synced ramp signal
