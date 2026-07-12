---
title: Silent Killer
description: "Sends a voice reset message when silence is detected after note-off, providing automatic voice cleanup."
factoryPath: envelope.silent_killer
factory: envelope
polyphonic: true
tags: [envelope, voice-management, utility]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "envelope.voice_manager", type: alternative, reason: "Preferred approach using explicit gate signals instead of silence detection" }
commonMistakes:
  - title: "Threshold parameter has no effect"
    wrong: "Adjusting the Threshold parameter expecting to control the silence detection sensitivity."
    right: "The silence detection uses a fixed internal threshold of approximately -90 dB. The Threshold parameter is stored but not used."
    explanation: "The per-block silence check uses its own hardcoded threshold. The Threshold parameter exists in the interface but does not influence the detection."
  - title: "Silence during note-on does not kill the voice"
    wrong: "Expecting the voice to be killed during brief silent passages while the note is held (e.g. from an LFO modulating amplitude to zero)."
    right: "The node only triggers a voice reset after a note-off has been received AND the audio is silent."
    explanation: "The node tracks note-on/off state internally. Silence detection only fires after note-off to avoid false kills during intentional momentary silences."
llmRef: |
  envelope.silent_killer

  Monitors the audio signal for silence and sends a voice reset message when the signal is silent after note-off. Audio passes through unmodified.

  Signal flow:
    audio in -> silence check -> audio out (passthrough)
    if silent and note-off received -> voice reset

  CPU: negligible, polyphonic

  Parameters:
    Threshold (-120 to -60 dB, default -100; stored but not used by the detection)
    Active (Off/On, default On; disables silence detection when Off)

  When to use:
    Fallback voice cleanup when the signal path makes it difficult to derive a clean gate signal. Prefer envelope.voice_manager with an explicit envelope gate for predictable behaviour.

  See also:
    [alternative] envelope.voice_manager -- preferred gate-based voice management
---

Monitors the audio signal and sends a voice reset message when silence is detected after a note-off event. The audio passes through completely unmodified. This provides automatic voice cleanup as a fallback when you cannot easily derive a clean gate signal from an envelope.

The node checks for silence on a per-block basis and only triggers when three conditions are met: the Active parameter is enabled, the voice has received a note-off, and the audio block is silent. The note-off requirement prevents false kills during intentional momentary silences (for example, from an LFO modulating amplitude to zero). The voice reset is immediate with no fade-out, but since the audio is already silent when the reset fires, no click occurs.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Active:
      desc: "Enables or disables silence detection"
      range: "Off / On"
      default: "On"
    Threshold:
      desc: "Stored but not used by the silence detection"
      range: "-120 - -60 dB"
      default: "-100.0"
  functions:
    silenceCheck:
      desc: "Per-block check whether all samples are below the internal threshold"
    voiceReset:
      desc: "Sends a voice reset message to stop the current voice"
---

```
// envelope.silent_killer - silence-based voice cleanup
// audio in -> audio out (passthrough)

process(input) {
    output = input                          // audio is never modified

    if (Active && noteOffReceived) {
        if (silenceCheck(input))             // all samples below ~-90 dB
            voiceReset()                     // kill this voice
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: Threshold, desc: "Intended as the silence detection threshold in decibels, but the detection uses a fixed internal threshold of approximately -90 dB. This parameter is stored but has no effect.", range: "-120 - -60 dB", default: "-100.0" }
      - { name: Active, desc: "Enables silence detection. When Off, the node becomes a transparent passthrough with no voice management.", range: "Off / On", default: "On" }
---
::

**See also:** $SN.envelope.voice_manager$ -- preferred gate-based voice management
