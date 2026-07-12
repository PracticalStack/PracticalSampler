---
title: Timer
description: "Generates a periodic modulation signal at a configurable interval, with selectable output modes."
factoryPath: control.timer
factory: control
polyphonic: true
tags: [control, timer, periodic, modulation]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: "Active", impact: "eliminates processing when off", note: "Early return when inactive" }
seeAlso:
  - { id: "core.clock_ramp", type: alternative, reason: "Tempo-synced continuous ramp rather than periodic triggers" }
  - { id: "control.tempo_sync", type: companion, reason: "Provides tempo-synced interval values" }
commonMistakes:
  - title: "Known crash in DAW plugins"
    wrong: "Using control.timer in a network that will be exported as a DAW plugin"
    right: "Replace control.timer with a tempo-synced ramp node and compare nodes to simulate the timer behaviour."
    explanation: "The control.timer node has been reported to crash both DAW plugins and standalone apps. A reliable workaround is to use a tempo-synced ramp with compare nodes to replicate the timer's engagement logic."
llmRef: |
  control.timer

  Generates a periodic modulation signal by counting audio samples against a configurable interval. The output value depends on the selected mode.

  Signal flow:
    Control node - processes in the audio callback
    Sample counter -> mode-dependent value -> normalised modulation output

  CPU: low, polyphonic (per-voice counter state)

  Parameters:
    Active (Off / On, default On): Enables or disables the timer
    Interval (0.0 - 2000.0 ms, default 500.0): Time between triggers

  Properties:
    Mode: Ping | Random | Toggle
    ClassId: Optional SNEX class for custom timer logic

  Known issues:
    Reported to crash DAW plugins and standalone apps. Workaround: replace with tempo-synced ramp + compare nodes.

  When to use:
    Use for periodic modulation events such as retriggering envelopes, generating random modulation, or creating square-wave LFOs at precise intervals. Caution: known crash reports in exported plugins.

  See also:
    [alternative] core.clock_ramp -- tempo-synced continuous ramp
    [companion] control.tempo_sync -- provides tempo-synced interval values
forumReferences:
  - { tid: 14414, summary: "control.timer crashes in DAW plugins -- workaround with ramp + compare" }
---

Timer generates a periodic modulation signal by counting audio samples against a configurable interval. Each time the interval elapses, it produces an output value determined by the selected Mode. This makes it useful for retriggering envelopes, generating periodic random modulation, or producing square-wave-style control signals.

The Mode property selects the output behaviour: Ping sends 1.0 on every tick, Random sends a new random value each tick, and Toggle alternates between 0.0 and 1.0. The timer runs in the audio callback, providing sample-accurate timing when used inside a frame processing container.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Active:
      desc: "Enables or disables the timer"
      range: "Off / On"
      default: "On"
    Interval:
      desc: "Time between triggers in milliseconds"
      range: "0.0 - 2000.0 ms"
      default: "500.0"
  functions:
    tick:
      desc: "Decrements the sample counter and fires when the interval has elapsed"
    getTimerValue:
      desc: "Returns the mode-dependent output value (1.0, random, or toggled)"
---

```
// control.timer - periodic modulation trigger
// internal counter -> normalised control out

perSample {
    if not Active:
        return

    tick(Interval)

    if counter has elapsed:
        output = getTimerValue()  // depends on Mode
        reset counter
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: ""
    params:
      - { name: Active, desc: "Enables the timer. When off, no processing occurs and no output is sent.", range: "Off / On", default: "On" }
      - { name: Interval, desc: "Time between triggers. Converted to samples internally for accurate timing.", range: "0.0 - 2000.0 ms", default: "500.0" }
---
::

### Timer Modes

The Mode property must be set before compilation:

- **Ping** -- sends 1.0 on every tick (periodic impulse)
- **Random** -- sends a new random value between 0.0 and 1.0 on every tick
- **Toggle** -- alternates between 0.0 and 1.0 on every tick (square wave)

Each voice maintains its own counter in polyphonic contexts, so voices started at different times will tick independently. When Active is toggled on, the counter resets and an initial value is sent immediately. The node also supports custom SNEX timer logic via the ClassId property, allowing user-defined output behaviour.

### Known Stability Issue

The control.timer node has been reported to crash both DAW plugins (e.g. Ableton Live) and standalone apps. If you encounter crashes, replace the timer with a tempo-synced ramp node and a pair of compare nodes to simulate the timer's engagement behaviour. This workaround avoids the crash entirely and provides equivalent functionality.

**See also:** $SN.core.clock_ramp$ -- tempo-synced continuous ramp rather than periodic triggers, $SN.control.tempo_sync$ -- provides tempo-synced interval values
