---
title: Tempo Sync
description: "Converts a musical tempo value to a duration in milliseconds and sends it as a modulation signal."
factoryPath: control.tempo_sync
factory: control
polyphonic: true
tags: [control, tempo, sync, time]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.transport", type: companion, reason: "Detects DAW play/stop state" }
  - { id: "control.ppq", type: companion, reason: "Provides PPQ position for playback-synced effects" }
  - { id: "core.clock_ramp", type: alternative, reason: "Ready-to-use tempo-synced ramp signal" }
commonMistakes:
  - title: "Forgetting output is in milliseconds"
    wrong: "Connecting tempo_sync directly to a frequency parameter"
    right: "Use a converter node (e.g. control.converter with Ms2Freq) between tempo_sync and a frequency target"
    explanation: "The output value is a duration in milliseconds. Parameters expecting Hz or other units need an explicit conversion step."
  - title: "Leaving Enabled off by default"
    wrong: "Expecting tempo-synced output without toggling Enabled on"
    right: "Set Enabled to On for tempo-synced operation, or use UnsyncedTime for manual control"
    explanation: "Enabled defaults to Off, which means the node outputs the UnsyncedTime value instead of the tempo-synced duration."
llmRef: |
  control.tempo_sync

  Converts a musical tempo value (e.g. 1/4, 1/8T) to a duration in milliseconds using the current DAW tempo. Outputs the raw millisecond value as an unnormalised modulation signal.

  Signal flow:
    Control node - no audio processing
    BPM + Tempo + Multiplier -> duration in ms -> unnormalised modulation output

  CPU: negligible, polyphonic

  Parameters:
    Tempo (0 - 18, default 0): Musical time value index (1/1 through 1/64T)
    Multiplier (1 - 16, default 1): Integer multiplier for the tempo duration
    Enabled (Off / On, default Off): Toggles between tempo-synced and manual time
    UnsyncedTime (0.0 - 1000.0 ms, default 200.0): Manual time used when Enabled is off

  When to use:
    Use to drive time-based effects (delay, LFO rate) with tempo-synced durations. Connect to a converter node if the target expects frequency rather than milliseconds.

  Common mistakes:
    Forgetting output is in milliseconds -- use a converter for frequency targets
    Leaving Enabled off by default -- toggle on for tempo sync

  See also:
    [companion] control.transport -- detects DAW play/stop state
    [companion] control.ppq -- provides PPQ position
    [alternative] core.clock_ramp -- ready-to-use tempo-synced ramp
---

Tempo Sync converts a musical time value (such as 1/4 note or 1/8 triplet) to a duration in milliseconds based on the current DAW tempo. The output updates whenever the host tempo changes or any parameter is adjusted, making it suitable for driving time-based effects that need to lock to the beat.

The output is an unnormalised modulation signal carrying the raw millisecond value. If the target parameter expects a different unit (such as frequency in Hz), place a [control.converter]($SN.control.converter$) between this node and the target. When Enabled is set to Off, the node outputs the manual UnsyncedTime value instead, allowing a smooth fallback for standalone operation or manual control.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Tempo:
      desc: "Musical time value (1/1, 1/2, 1/4, etc.)"
      range: "0 - 18"
      default: "0"
    Multiplier:
      desc: "Integer multiplier applied to the tempo duration"
      range: "1 - 16"
      default: "1"
    Enabled:
      desc: "Toggles between tempo-synced and manual time"
      range: "Off / On"
      default: "Off"
    UnsyncedTime:
      desc: "Manual time value used when sync is disabled"
      range: "0.0 - 1000.0 ms"
      default: "200.0"
  functions:
    tempoToMs:
      desc: "Calculates duration in milliseconds from BPM and musical time value"
---

```
// control.tempo_sync - musical time to milliseconds
// BPM + parameters -> ms out (unnormalised)

onParameterChange() {
    if Enabled:
        output = tempoToMs(bpm, Tempo) * Multiplier
    else:
        output = UnsyncedTime
}

onTempoChange(newBpm) {
    bpm = newBpm
    if Enabled:
        output = tempoToMs(bpm, Tempo) * Multiplier
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Tempo
    params:
      - { name: Tempo, desc: "Musical time value. Selects from standard note durations: 1/1, 1/2, 1/2T, 1/4, 1/4T, 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/32T, 1/64, 1/64T and dotted variants.", range: "0 - 18", default: "0" }
      - { name: Multiplier, desc: "Integer multiplier applied to the calculated duration. Allows non-standard rates such as 3/16 (1/16 x 3).", range: "1 - 16", default: "1" }
  - label: Configuration
    params:
      - { name: Enabled, desc: "Toggles between tempo-synced and manual mode. When off, the output uses UnsyncedTime instead.", range: "Off / On", default: "Off" }
      - { name: UnsyncedTime, desc: "Manual time in milliseconds, used when Enabled is off.", range: "0.0 - 1000.0 ms", default: "200.0" }
---
::

The output only updates when the calculated value actually changes, avoiding redundant modulation signals. Each voice maintains its own state in polyphonic contexts, though the underlying BPM and parameters are shared.

**See also:** $SN.control.transport$ -- detects DAW play/stop state, $SN.control.ppq$ -- provides PPQ position for playback-synced effects, $SN.core.clock_ramp$ -- ready-to-use tempo-synced ramp signal
