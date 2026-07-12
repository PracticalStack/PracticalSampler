---
title: Bang
description: "Sends a stored value to the modulation output when triggered by a rising edge on the Bang parameter."
factoryPath: control.bang
factory: control
polyphonic: true
tags: [control, trigger, sample-and-hold]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.change", type: companion, reason: "Filters duplicate values -- useful before a bang trigger" }
  - { id: "control.voice_bang", type: alternative, reason: "Voice-specific trigger variant" }
commonMistakes:
  - title: "Bang requires a value above 0.5"
    wrong: "Connecting a smooth modulation signal that hovers near 0.0 to the Bang input and expecting a trigger"
    right: "Ensure the Bang input receives a clear transition above 0.5. A button or gate signal works best."
    explanation: "The Bang parameter only fires when its value exceeds 0.5. Modulation signals that stay below the threshold will never trigger output."
llmRef: |
  control.bang

  Sends a stored value to the modulation output when triggered. The Value parameter holds the output value (unnormalised). The Bang parameter triggers on any value above 0.5.

  Signal flow:
    Control node -- no audio processing
    Value param -> stored | Bang param (> 0.5) -> trigger -> send stored Value to modulation output

  CPU: negligible, polyphonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). The value to send when triggered. Unnormalised -- the raw value is forwarded without range conversion.
    Bang: 0.0 - 1.0 (default 0.0, step 1.0). Trigger input. Values above 0.5 fire the output.

  When to use:
    Retiming a modulation signal -- store a value and emit it in response to a separate trigger event. 7 instances across surveyed projects (rank 50). Useful for sample-and-hold patterns where one signal determines the value and another determines when it is sent.

  See also:
    [companion] control.change -- filters duplicate values before triggering
    [alternative] control.voice_bang -- voice-specific trigger
---

Stores a value and sends it to the modulation output when the Bang parameter receives a trigger. This acts as a sample-and-hold mechanism: the Value parameter determines what is sent, while the Bang parameter determines when it is sent. The output carries the raw value with no range conversion applied.

Each voice maintains independent value and trigger state in polyphonic mode. Only the voice currently being rendered receives the output when the trigger fires.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "The value to send when triggered (unnormalised)"
      range: "0.0 - 1.0"
      default: "0.0"
    Bang:
      desc: "Trigger input -- fires when above 0.5"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// control.bang - trigger-based value sender
// control in -> stored value out on trigger

onBangReceived(Bang) {
    if (Bang > 0.5)
        sendToOutput(Value)     // sends stored Value unchanged
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "The value to send when triggered. Receives raw values -- the modulation output forwards this without range conversion.", range: "0.0 - 1.0", default: "0.0" }
  - label: Trigger
    params:
      - { name: Bang, desc: "Trigger input. Any value above 0.5 fires the stored Value to the modulation output.", range: "Off / On", default: "Off" }
---
::

Changing the Value parameter while no trigger is active simply updates the stored value without producing output. The output only fires when the Bang parameter is set above 0.5. Repeated values above the threshold will re-trigger the output each time.

**See also:** $SN.control.change$ -- filters duplicate values, $SN.control.voice_bang$ -- voice-specific trigger variant
