---
title: Voice Manager
description: "Sends a voice reset message when the input value drops below 0.5, providing gate-based voice lifecycle control."
factoryPath: envelope.voice_manager
factory: envelope
polyphonic: false
tags: [envelope, voice-management, utility]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "envelope.ahdsr", type: companion, reason: "Connect its Gate output to Kill Voice for standard voice management" }
  - { id: "envelope.silent_killer", type: alternative, reason: "Silence-based fallback when a clean gate signal is unavailable" }
commonMistakes:
  - title: "Connect an envelope Gate output to Kill Voice"
    wrong: "Leaving the Kill Voice parameter at its default without connecting a modulation source."
    right: "Wire the Gate output of an envelope node (e.g. envelope.ahdsr) to the Kill Voice parameter so voices are stopped when the envelope finishes."
    explanation: "The default value of 1.0 means no voice is ever killed. The node only acts when a modulation source drives the value below 0.5."
llmRef: |
  envelope.voice_manager

  Receives a modulation value and kills the currently-rendering voice when it drops below 0.5. Does not process audio. Typically driven by an envelope's Gate output.

  Signal flow:
    Control node - no audio processing
    Kill Voice value < 0.5 -> voice reset for current voice

  CPU: negligible, monophonic (but operates within polyphonic voice context)

  Parameters:
    Kill Voice (0 or 1, default 1.0; value < 0.5 kills the current voice)

  When to use:
    Standard voice lifecycle management. Place in the signal chain and connect an envelope's Gate output to Kill Voice.

  Common mistakes:
    - Must be connected to a modulation source; default value never triggers

  See also:
    [companion] envelope.ahdsr -- connect its Gate output here
    [alternative] envelope.silent_killer -- silence-based fallback
---

Sends a voice reset message when the Kill Voice parameter drops below 0.5. This is the standard way to manage voice lifecycle in scriptnode: connect an envelope's Gate modulation output to the Kill Voice parameter, and the voice is stopped once the envelope finishes its release phase and goes idle.

The node does not process audio and sits outside the signal path. Despite being monophonic, it operates within the polyphonic voice rendering context, so the voice reset targets whichever voice is currently being rendered. The Kill Voice parameter uses a step size of 1, so the UI presents it as a simple toggle (0 or 1), but any value below 0.5 from a modulation source triggers the kill. The node displays the number of active voices in its custom editor and provides a panic button for manually resetting all voices.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Kill Voice:
      desc: "When the value drops below 0.5, the currently-rendering voice is killed"
      range: "0 - 1"
      default: "1.0"
  functions:
    voiceReset:
      desc: "Sends a voice reset message to stop the current voice"
---

```
// envelope.voice_manager - gate-based voice lifecycle
// control node, no audio processing

onValueChange(Kill Voice) {
    if (Kill Voice < 0.5)
        voiceReset()                // kill the current voice
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Control
    params:
      - { name: Kill Voice, desc: "Triggers a voice reset when the value drops below 0.5. Connect an envelope's Gate modulation output here. The default of 1.0 means no kill; a value of 0 kills the voice.", range: "0 - 1", default: "1.0" }
---
::

**See also:** $SN.envelope.ahdsr$ -- connect its Gate output here, $SN.envelope.silent_killer$ -- silence-based fallback
