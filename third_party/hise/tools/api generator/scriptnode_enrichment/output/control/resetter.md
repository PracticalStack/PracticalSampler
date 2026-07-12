---
title: Resetter
description: "Sends a 0-then-1 impulse pair to the modulation output on any input change, used to re-trigger gate-like parameters."
factoryPath: control.resetter
factory: control
polyphonic: false
tags: [control, trigger, reset, gate]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.bang", type: alternative, reason: "Sends a stored value on trigger rather than a fixed impulse pair" }
commonMistakes:
  - title: "Input value is ignored"
    wrong: "Sending specific values to the Value parameter expecting them to influence the output"
    right: "The input value is completely ignored. Any change fires the same 0-then-1 impulse pair."
    explanation: "The Value parameter acts purely as a trigger. The output always sends 0.0 followed by 1.0 regardless of what value was received."
llmRef: |
  control.resetter

  Sends a 0-then-1 impulse pair to re-trigger gate-like parameters. Any change to Value fires the impulse -- the input value is ignored.

  Signal flow:
    Control node -- no audio processing
    Value change (any) -> send 0.0 -> send 1.0 -> modulation output

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Trigger input. The value itself is ignored; any change fires the 0 -> 1 impulse.

  When to use:
    Forcing a re-trigger on envelope gates or toggle parameters regardless of their current state. 2 instances in surveyed projects (rank 87). Connect to an envelope's gate parameter to restart the envelope on demand.

  Common mistakes:
    Input value is ignored -- it is purely a trigger.

  See also:
    [alternative] control.bang -- sends a stored value on trigger
---

Sends a rapid 0-then-1 impulse pair to the modulation output whenever the Value parameter changes. The input value is completely ignored -- any change fires the same impulse sequence. This is designed to re-trigger gate-like parameters (such as envelope gates) regardless of their current state.

The 0-then-1 sequence ensures a clean rising edge: the target parameter first receives 0.0 (off), then immediately receives 1.0 (on). This forces a re-trigger even if the target was already in the "on" state.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Trigger input -- any change fires the impulse pair (input value ignored)"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    sendImpulse:
      desc: "Sends 0.0 then 1.0 in immediate succession to force a rising edge"
---

```
// control.resetter - gate re-trigger impulse
// trigger in -> 0/1 impulse pair out

onValueChange(input) {
    // input value is ignored
    sendImpulse()
    // sends 0.0 then 1.0 to modulation output
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Trigger input. The value itself is ignored -- any change sends a 0.0 followed by 1.0 to the modulation output, forcing a rising edge on the target.", range: "0.0 - 1.0", default: "0.0" }
---
::

Connect the output to an envelope gate parameter to restart the envelope on demand. The 0-then-1 sequence works with any parameter that responds to rising edges or state transitions.

**See also:** $SN.control.bang$ -- sends a stored value on trigger rather than a fixed impulse pair
