---
title: Smoother
description: "A one-pole lowpass filter that smooths channel 0 of the input signal."
factoryPath: core.smoother
factory: core
polyphonic: true
tags: [core, smoothing, filter, utility]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
forumReferences:
  - { tid: 5694, reason: "Buffer-rate smoothing gotcha and fix32_block workaround" }
seeAlso:
  - { id: "filters.one_pole", type: alternative, reason: "Full one-pole filter processing all channels" }
commonMistakes:
  - title: "Only channel 0 is smoothed"
    wrong: "Expecting the smoother to process all channels equally"
    right: "Only channel 0 is filtered. Other channels pass through unmodified. Use filters.one_pole if you need all channels smoothed."
    explanation: "The node applies its lowpass filter to channel 0 only. In a stereo signal, the right channel is untouched."
  - title: "Updates once per buffer without a fixed-block wrapper"
    wrong: "Using core.smoother to smooth a rapidly modulated parameter and hearing zipper noise"
    right: "Wrap the processing chain in a container.fix32_block (or similar fixed-block container) to force per-sample updates."
    explanation: "By default, the smoother updates once per audio buffer. For parameters that change frequently, this still produces audible stepping. A fixed-block or frame-processing wrapper forces sample-accurate smoothing and also ensures correct results during offline bounce."
llmRef: |
  core.smoother

  One-pole lowpass filter applied to channel 0 only. Smooths rapid changes to reduce zipper noise or create gradual transitions. Resets to DefaultValue on voice start or note-on.

  Signal flow:
    channel 0 -> one-pole lowpass -> channel 0 (other channels pass through)

  CPU: negligible, polyphonic

  Parameters:
    SmoothingTime (0-2000 ms, default 100): Filter time constant
    DefaultValue (0-100%, default 0%): Initial output value on voice start/reset

  When to use:
    - Smoothing a control signal or modulation value on channel 0
    - Reducing zipper noise from stepped parameter changes
    - Use filters.one_pole instead when all channels need filtering

  Common mistakes:
    - Only channel 0 is processed; other channels are untouched
    - Updates once per buffer by default -- wrap in fix32_block for sample-accurate smoothing

  Forum references: tid:5694 (buffer-rate update and fix32 workaround)

  See also:
    alternative filters.one_pole -- one-pole filter for all channels
---

The smoother applies a one-pole lowpass filter to channel 0 of the input signal, reducing rapid changes and creating smooth transitions. It is commonly used to smooth control signals or reduce zipper noise from stepped modulation values.

On voice start or MIDI note-on, the filter state resets to the DefaultValue, then tracks the input signal with the configured time constant. Only channel 0 is filtered; other channels pass through unmodified.

## Signal Path

::signal-path
---
glossary:
  parameters:
    SmoothingTime:
      desc: "Filter time constant controlling how quickly the output tracks the input"
      range: "0 - 2000 ms"
      default: "100"
    DefaultValue:
      desc: "Initial output value on voice start or reset"
      range: "0 - 100%"
      default: "0%"
  functions:
    onePoleFilter:
      desc: "Applies exponential smoothing to each sample, gradually approaching the input value"
---

```
// core.smoother - one-pole lowpass on channel 0
// audio in -> audio out (channel 0 filtered)

process(input) {
    input[ch0] = onePoleFilter(input[ch0], SmoothingTime)
    // other channels pass through unchanged
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: SmoothingTime, desc: "Filter time constant in milliseconds. Higher values produce slower, smoother transitions", range: "0 - 2000 ms", default: "100" }
      - { name: DefaultValue, desc: "Initial output value applied when a voice starts or the node is reset. The output begins at this value and then smoothly transitions to track the input", range: "0 - 100%", default: "0%" }
---
::

### Sample-accurate smoothing

By default, the smoother updates once per audio buffer. If parameters are being modulated at high speed (for example, from an automation lane or an audio-rate source), this can produce audible zipper noise. To force per-sample updates, wrap the surrounding processing chain in a [container.fix32_block]($SN.container.fix32_block$) or a frame-processing container. This also ensures correct behaviour during offline bounce, where the buffer size may differ from real-time playback.

**See also:** $SN.filters.one_pole$ -- one-pole filter processing all channels
