---
title: Delay
moduleId: Delay
type: Effect
subtype: MasterEffect
tags: [delay]
builderPath: b.Effects.Delay
screenshot: /images/v2/reference/audio-modules/delay.png
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: PhaseFX, type: alternative, reason: "Allpass-based phaser for notch sweeps rather than echo repeats" }
  - { id: Chorus, type: alternative, reason: "Short modulated delay for chorus/thickening rather than rhythmic echoes" }
  - { id: SimpleGain, type: companion, reason: "SimpleGain includes a static delay feature useful for timing alignment without feedback" }
  - { id: "core.fix_delay", type: scriptnode, reason: "Scriptnode fixed-size delay line (no feedback or tempo sync -- build those with send/receive)" }
  - { id: "jdsp.jdelay", type: scriptnode, reason: "Scriptnode JUCE-based delay line with interpolation options" }
forumReferences:
  - id: 1
    title: "1ms granularity; use scriptnode for sub-millisecond delays"
    summary: "The module tree Delay effect only accepts integer millisecond values; for sample-accurate or sub-millisecond delays the scriptnode delay node must be used instead."
    topic: 11516
commonMistakes:
  - title: "Filter parameters have no effect"
    wrong: "Adjusting the Low-Pass or High-Pass filter parameters expecting them to shape the delay feedback tone"
    right: "These parameters have no effect on the audio output"
    explanation: "The LowPassFreq and HiPassFreq parameters are defined in the interface but are not connected to any filtering. They appear in the metadata but do not affect processing."
  - title: "1.0 feedback causes infinite accumulation"
    wrong: "Setting FeedbackLeft or FeedbackRight to 1.0 expecting infinite sustain"
    right: "A feedback of 1.0 produces infinite repeats that never decay. The signal will accumulate and eventually clip."
    explanation: "Unlike some delay plugins that limit feedback below unity, this delay allows true 1.0 feedback. Use values below 0.95 for repeats that decay naturally."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: simple
  description: "Delay line nodes with feedback and tempo sync in a scriptnode network"
llmRef: |
  Delay (MasterEffect)

  Stereo delay with independent left/right times, feedback, and optional tempo sync. Uses an overlap fader for dry/wet mixing. The LowPassFreq and HiPassFreq parameters are defined but vestigial - they have no effect on audio.

  Signal flow:
    audio in -> per-sample: dryMix * sample + wetMix * delayLine(sample + lastOutput * feedback) -> audio out

  CPU: medium, monophonic (MasterEffect).

  Parameters:
    DelayTimeLeft (0-3000 ms or tempo-synced, default QuarterTriplet synced) - left channel delay time. 1ms granularity; max depends on sample rate (~2730ms at 48kHz, ~1365ms at 96kHz).
    DelayTimeRight (0-3000 ms or tempo-synced, default Quarter synced) - right channel delay time. Same limitations as left.
    FeedbackLeft (0-100%, default 30%) - left channel feedback
    FeedbackRight (0-100%, default 30%) - right channel feedback
    LowPassFreq (20-20000 Hz, default 20000 Hz) - has no effect (vestigial)
    HiPassFreq (20-20000 Hz, default 40 Hz) - has no effect (vestigial)
    Mix (0-100%, default 50%) - overlap fader dry/wet blend
    TempoSync (Off/On, default On) - sync delay times to host tempo

  When to use:
    Rhythmic echo effects, ping-pong style delays (using different L/R times), slapback delays.

  Common mistakes:
    LowPassFreq and HiPassFreq have no effect.
    Feedback of 1.0 produces infinite non-decaying repeats.

  Custom equivalent:
    scriptnode HardcodedFX: delay line nodes with feedback and tempo sync.

  See also:
    alternative PhaseFX - allpass notch sweeps
    alternative Chorus - short modulated delay for thickening
    companion SimpleGain - static delay for timing alignment
    [scriptnode] core.fix_delay -- scriptnode fixed-size delay line (no feedback or tempo sync -- build those with send/receive)
    scriptnode jdsp.jdelay -- JUCE-based delay line with interpolation options
---

::category-tags
---
tags:
  - { name: delay, desc: "Effects based on delayed signal copies, including chorus and phaser" }
---
::

![Delay screenshot](/images/v2/reference/audio-modules/delay.png)

A zero-latency stereo delay effect with independent left and right delay times, feedback controls, and optional tempo synchronisation. Each channel has its own delay line with adjustable feedback for controlling the number of repeats. The dry/wet mix uses an overlap fader that keeps the combined level more consistent than a linear crossfade. The first audio buffer after initialisation is skipped to avoid outputting uninitialised delay buffer contents.

When tempo sync is enabled (the default), the delay times snap to musical note values from whole notes down to 1/64th triplets. The delay crossfades between old and new read positions when the time changes, preventing clicks during automation or tempo changes.

### Timing Limitations

Delay times are rounded to whole milliseconds (1ms granularity). For sub-millisecond or sample-accurate delays, use a scriptnode delay node instead. [1]($FORUM_REF.11516$)

The internal delay buffer has a fixed size of 131072 samples, which limits the achievable maximum delay time depending on the host sample rate. At 48kHz, the maximum is approximately 2730ms rather than the 3000ms shown in the parameter range. At 96kHz, the maximum drops to approximately 1365ms.

## Signal Path

::signal-path
---
glossary:
  parameters:
    DelayTimeLeft:
      desc: "Left channel delay time in ms or synced note value"
      range: "0 - 3000 ms"
      default: "(synced)"
    DelayTimeRight:
      desc: "Right channel delay time in ms or synced note value"
      range: "0 - 3000 ms"
      default: "(synced)"
    FeedbackLeft:
      desc: "Left channel feedback amount"
      range: "0 - 100%"
      default: "30%"
    FeedbackRight:
      desc: "Right channel feedback amount"
      range: "0 - 100%"
      default: "30%"
    Mix:
      desc: "Overlap fader dry/wet blend"
      range: "0 - 100%"
      default: "50%"
    TempoSync:
      desc: "Sync delay times to host tempo"
      range: "Off / On"
      default: "On"
  functions:
    overlapMix:
      desc: "Overlap fader blending that keeps combined level more consistent than a linear crossfade"
  modulations: {}
---

```
// Delay - monophonic stereo delay
// stereo in -> stereo out

process(left, right) {
    // Delay time (ms or synced to host BPM)
    if TempoSync:
        timeL = tempoToMs(HostBPM, DelayTimeLeft)
        timeR = tempoToMs(HostBPM, DelayTimeRight)
    else:
        timeL = DelayTimeLeft
        timeR = DelayTimeRight

    // Per-sample feedback delay (each channel independent)
    wetL = delayLine(left + lastOutputL * FeedbackLeft, timeL)
    wetR = delayLine(right + lastOutputR * FeedbackRight, timeR)

    // Overlap fader mix
    left  = overlapMix(left, wetL, Mix)
    right = overlapMix(right, wetR, Mix)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Delay Time
    params:
      - name: DelayTimeLeft
        desc: "Left channel delay time. In milliseconds when Tempo Sync is off, or a musical note value when on. Default sync value is quarter-note triplet."
        range: "0 - 3000 ms"
        default: "(synced)"
        hints:
          - type: warning
            text: "Values are rounded to whole milliseconds (1ms granularity). For sub-millisecond or sample-accurate delays, use a scriptnode delay node instead."
          - type: warning
            text: "The actual maximum depends on sample rate: ~2970ms at 44.1kHz, ~2730ms at 48kHz, ~1365ms at 96kHz. Values above the limit are silently clamped."
      - name: DelayTimeRight
        desc: "Right channel delay time. In milliseconds when Tempo Sync is off, or a musical note value when on. Default sync value is quarter note."
        range: "0 - 3000 ms"
        default: "(synced)"
        hints:
          - type: warning
            text: "Same limitations as DelayTimeLeft: 1ms granularity, sample-rate-dependent maximum."
      - { name: TempoSync, desc: "When enabled, delay times are interpreted as musical note values synchronised to the host tempo. Available values range from whole notes to 1/64th triplets, including dotted and triplet variants.", range: "Off / On", default: "On" }
  - label: Feedback
    params:
      - { name: FeedbackLeft, desc: "Controls how much of the delayed left signal is fed back into the delay line. At 0% there are no repeats. At 100% the repeats sustain indefinitely.", range: "0 - 100%", default: "30%" }
      - { name: FeedbackRight, desc: "Controls how much of the delayed right signal is fed back into the delay line. At 0% there are no repeats. At 100% the repeats sustain indefinitely.", range: "0 - 100%", default: "30%" }
  - label: Filter (Vestigial)
    params:
      - { name: LowPassFreq, desc: "This parameter has no effect.", range: "20 - 20000 Hz", default: "20000 Hz" }
      - { name: HiPassFreq, desc: "This parameter has no effect.", range: "20 - 20000 Hz", default: "40 Hz" }
  - label: Mix
    params:
      - { name: Mix, desc: "Dry/wet balance using an overlap fader. At 0% the output is fully dry. At 100% the output is fully wet. The overlap fader keeps the combined level more consistent than a simple linear crossfade.", range: "0 - 100%", default: "50%" }
---
::

**See also:** $MODULES.PhaseFX$ -- Allpass-based phaser for notch sweeps rather than echo repeats, $MODULES.Chorus$ -- Short modulated delay for chorus/thickening rather than rhythmic echoes, $MODULES.SimpleGain$ -- Includes a static delay feature useful for timing alignment without feedback, $SN.core.fix_delay$ -- scriptnode fixed-size delay line (no feedback or tempo sync -- build those with send/receive), $SN.jdsp.jdelay$ -- Scriptnode JUCE-based delay line with interpolation options
