---
title: File Analyser
description: "Extracts pitch, duration, or peak level from an audio file and sends the result as a modulation signal when the file is loaded."
factoryPath: control.file_analyser
factory: control
polyphonic: false
tags: [control, analysis, audio-file]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Analysis only runs on file load"
    wrong: "Expecting the node to continuously analyse during playback"
    right: "The analysis runs once when the audio file is loaded or changed. It does not update during playback."
    explanation: "The node is event-driven: it triggers only when an audio file is assigned. For real-time analysis, use a different approach such as the analyse factory nodes."
llmRef: |
  control.file_analyser

  Extracts a property (pitch, duration, or peak level) from an audio file and sends it as an unnormalised modulation signal when the file is loaded.

  Signal flow:
    Control node -- no audio processing
    Audio file loaded -> analyse based on Mode -> send raw value to modulation output

  CPU: negligible (runs only on file load), monophonic

  Properties:
    Mode: Peak | milliseconds | pitch. Selects which property to extract from the audio file.

  When to use:
    Automatically configuring downstream parameters based on audio file properties -- for example, setting a delay time from the file duration or tuning an oscillator from the detected pitch. Not observed in the surveyed projects.

  See also:
    (none)
---

Extracts a single property from a loaded audio file and sends the result as an unnormalised modulation signal. The analysis runs once when the audio file is assigned or changed -- it does not process during playback.

The Mode property determines which property is extracted:

- **Peak** -- the peak amplitude of the file as a linear value (typically 0.0 to 1.0)
- **milliseconds** -- the duration of the file in milliseconds
- **pitch** -- the detected fundamental frequency in Hz

## Signal Path

::signal-path
---
glossary:
  functions:
    analyse:
      desc: "Extracts the selected property from the audio file data"
---

```
// control.file_analyser - extracts file properties on load
// audio file -> analysis result out

onFileLoad(audioFile) {
    result = analyse(audioFile)     // based on Mode: peak, ms, or Hz
    if (result != 0)
        sendToOutput(result)        // raw unnormalised value
}
```

::

### Output Values

The output values are unnormalised (raw). For the **pitch** mode, the output is a frequency in Hz (e.g. 440.0 for A4). For **milliseconds**, the output is the file duration in ms. For **peak**, the output is the linear peak amplitude. Connect the output to a node such as [control.pma_unscaled]($SN.control.pma_unscaled$) or [control.converter]($SN.control.converter$) if you need to scale or transform the raw value before it reaches a target parameter.

If the audio file is empty (zero length), no modulation signal is sent.
