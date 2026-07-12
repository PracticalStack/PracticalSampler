---
title: Transport
description: "Sends a modulation signal when the DAW transport state changes between playing and stopped."
factoryPath: control.transport
factory: control
polyphonic: true
tags: [control, transport, daw, host]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.ppq", type: companion, reason: "Provides PPQ position for playback-synced effects" }
  - { id: "core.clock_ramp", type: companion, reason: "Tempo-synced ramp signal driven by host playback" }
  - { id: "control.tempo_sync", type: companion, reason: "Converts host tempo to millisecond durations" }
llmRef: |
  control.transport

  Outputs a normalised modulation signal reflecting the DAW transport state: 1.0 when playing, 0.0 when stopped.

  Signal flow:
    Control node - no audio processing
    DAW transport state -> 1.0 (playing) or 0.0 (stopped) -> normalised modulation output

  CPU: negligible, polyphonic

  Parameters:
    None

  When to use:
    Use to enable or disable processing based on whether the host is playing. Connect to a soft bypass, gate, or any parameter that should respond to transport state.

  See also:
    [companion] control.ppq -- provides PPQ position
    [companion] core.clock_ramp -- tempo-synced ramp signal
    [companion] control.tempo_sync -- converts host tempo to ms durations
---

Transport outputs a normalised modulation signal that reflects the DAW's play/stop state. It sends 1.0 when the host starts playing and 0.0 when playback stops. This allows any downstream parameter to respond to transport changes - for example, enabling an effect only during playback or gating a signal when the host is stopped.

The node has no parameters and requires no configuration. It listens to transport state changes from the host and updates its output only when the state actually changes, avoiding redundant signals.

## Signal Path

::signal-path
---
glossary:
  functions:
    onTransportChange:
      desc: "Fires when the DAW transport starts or stops, updating the output value"
---

```
// control.transport - DAW play/stop state
// host transport -> normalised control out

onTransportChange(isPlaying) {
    if isPlaying:
        output = 1.0
    else:
        output = 0.0
}
```

::

Each voice independently detects the transport change in polyphonic contexts, ensuring every active voice receives the state update exactly once.

**See also:** $SN.control.ppq$ -- provides PPQ position for playback-synced effects, $SN.core.clock_ramp$ -- tempo-synced ramp signal driven by host playback, $SN.control.tempo_sync$ -- converts host tempo to millisecond durations
