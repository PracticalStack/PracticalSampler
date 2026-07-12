---
title: Delay Cable
description: "Delays a control signal by a configurable number of samples before forwarding it to the modulation output."
factoryPath: control.delay_cable
factory: control
polyphonic: true
tags: [control, delay, timing]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.smoothed_parameter", type: alternative, reason: "Smooths value transitions rather than delaying them" }
commonMistakes:
  - title: "Minimum delay is one audio block"
    wrong: "Setting DelayTimeSamples to 0 and expecting the value to arrive in the same callback"
    right: "Even with DelayTimeSamples at 0, the value arrives on the next audio processing block."
    explanation: "The sample counter is checked during audio processing. A delay of zero still requires at least one process call before the value is forwarded."
  - title: "Only one value can be queued at a time"
    wrong: "Sending multiple value changes in quick succession and expecting all of them to arrive"
    right: "Each new value resets the delay counter. Only the most recently set value is forwarded after the delay elapses."
    explanation: "Setting Value again while a previous value is still waiting replaces the queued value and restarts the counter from zero."
llmRef: |
  control.delay_cable

  Delays a control value by a configurable number of samples before forwarding it. Only one value can be in flight at a time; new values reset the counter.

  Signal flow:
    Control node -- requires audio processing for sample counting
    Value in -> store + reset counter -> count samples in process() -> when elapsed, forward to modulation output

  CPU: negligible, polyphonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). The value to delay. Unnormalised -- the raw value is forwarded.
    DelayTimeSamples: 0 - 44100 samples (default 0.0). Number of samples to wait before forwarding.

  When to use:
    When a control signal needs to arrive later than its source fires -- for example, staggering parameter changes or introducing timing offsets in modulation routing. Not observed in the surveyed projects, suggesting it is a specialised utility.

  See also:
    [alternative] control.smoothed_parameter -- smooths rather than delays
---

Delays a control value by a specified number of samples before forwarding it to the modulation output. When Value changes, the node stores the new value and begins counting samples during audio processing. Once the count reaches the DelayTimeSamples threshold, the stored value is sent to the output.

Unlike most control nodes, delay_cable participates in audio processing because it needs to count samples. Only one value can be queued at a time -- setting Value again while waiting replaces the queued value and restarts the counter.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "The value to delay (unnormalised)"
      range: "0.0 - 1.0"
      default: "0.0"
    DelayTimeSamples:
      desc: "Number of samples to wait before forwarding"
      range: "0 - 44100 samples"
      default: "0.0"
  functions:
    countSamples:
      desc: "Increments the sample counter each audio block until the delay threshold is reached"
---

```
// control.delay_cable - delays a control value by N samples
// control in -> delayed control out

onValueChange(input) {
    storedValue = input
    sampleCounter = 0       // reset counter
}

process() {
    countSamples()          // increment by block size each call
    if (sampleCounter >= DelayTimeSamples)
        sendToOutput(storedValue)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "The value to delay. Receives raw values -- the modulation output forwards this without range conversion.", range: "0.0 - 1.0", default: "0.0" }
  - label: Configuration
    params:
      - { name: DelayTimeSamples, desc: "Number of samples to wait before forwarding the value to the output. Changing this while a value is queued affects the remaining wait time.", range: "0 - 44100 samples", default: "0.0" }
---
::

The delay is measured in raw samples, not milliseconds. At 44100 Hz, the maximum delay of 44100 samples equals one second. Modulating DelayTimeSamples in real time is supported: shortening it below the current counter causes immediate output on the next process call, while lengthening it extends the wait.

**See also:** $SN.control.smoothed_parameter$ -- smooths value transitions rather than delaying them
