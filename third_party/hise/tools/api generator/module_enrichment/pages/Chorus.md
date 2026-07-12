---
title: Chorus
moduleId: Chorus
type: Effect
subtype: MasterEffect
tags: [delay]
builderPath: b.Effects.Chorus
screenshot: /images/v2/reference/audio-modules/chorus.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: PhaseFX, type: alternative, reason: "Uses allpass filters for frequency notch sweeping rather than delay-line pitch modulation" }
  - { id: "jdsp.jchorus", type: scriptnode, reason: "Scriptnode LFO-modulated delay chorus with configurable mix and parameters" }
commonMistakes:
  - title: "Chorus lacks stereo LFO offset"
    wrong: "Expecting stereo widening from the chorus"
    right: "Both channels share the same internal LFO phase, so there is no inherent stereo widening from phase differences"
    explanation: "The stereo effect comes only from different feedback histories in each channel's delay buffer. For stronger stereo widening, consider using two instances with different settings or a scriptnode alternative."
  - title: "Feedback 0.5 equals zero feedback"
    wrong: "Setting Feedback to 0.5 expecting moderate feedback"
    right: "At Feedback=0.5 the internal feedback is actually zero. Values below 0.5 produce negative feedback, values above 0.5 produce positive feedback."
    explanation: "The Feedback parameter maps 0-1 to an internal range of -0.95 to +0.95. The crossover point at 0.5 means zero feedback."
  - title: "No Mix parameter"
    wrong: "Looking for a wet/dry mix control"
    right: "The wet/dry mix is hardcoded at approximately 47% wet"
    explanation: "There is no Mix parameter. To blend the effect, use send/return routing or bypass the module programmatically."
forumReferences:
  - id: 1
    title: "Wet/dry mix is hardcoded at 47% — no Mix parameter"
    summary: "The wet level is fixed at 0.47 in the constructor and cannot be changed. Use send/return routing to control blend."
    topic: 4525
  - id: 2
    title: "Both channels share the same LFO phase — no stereo widening"
    summary: "The internal LFO uses identical phase for both L and R delay lines; stereo effect comes only from different feedback histories, not LFO offset."
    topic: 4525
  - id: 3
    title: "Wet signal is subtracted from dry — subtractive comb-filter mixing"
    summary: "Output is dry * input minus wet * delayed, not additive. This creates a comb-filter cancellation pattern characteristic of the mda chorus algorithm."
    topic: 980
  - id: 4
    title: "Use scriptnode jchorus node for a more flexible chorus"
    summary: "Multiple experienced users recommend the scriptnode jchorus node when wider stereo, independent L/R LFO phases, or configurable mix are needed."
    topic: 12406
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: simple
  description: "A modulated delay line with feedback in a scriptnode network provides more control over LFO shape, stereo offset, and wet/dry mix"
llmRef: |
  Chorus (MasterEffect)

  Simple stereo chorus with an internal parabolic LFO modulating a delay line. The wet/dry mix is hardcoded at 47% wet / 53% dry and cannot be adjusted. Both channels share the same LFO phase (no stereo offset).

  Signal flow:
    audio in -> write (input + feedback * lastDelayed) to buffer -> read at LFO-modulated position (linear interpolation) -> subtractive mix (53% dry - 47% wet) -> audio out

  CPU: low, monophonic (MasterEffect).

  Parameters:
    Rate (0-100%, default 30%) - LFO speed, approximately 0.01-10 Hz logarithmic
    Width (0-100%, default 43%) - modulation depth controlling pitch variation intensity
    Feedback (0-100%, default 30%) - maps to internal -0.95 to +0.95 range. Default gives negative feedback (-0.38).
    Delay (0-100%, default 100%) - shifts the modulation centre point. At 100%, full depth range is used.

  When to use:
    Quick chorus thickening effect with minimal CPU. For more control over stereo width, wet/dry mix, or LFO shape, use the scriptnode equivalent.

  Common mistakes:
    No stereo widening from LFO phase offset - both channels share the same phase.
    Feedback=0.5 is zero feedback, not moderate. Below 0.5 = negative, above 0.5 = positive.
    Wet/dry mix is fixed at 47/53 and cannot be changed.

  Custom equivalent:
    scriptnode HardcodedFX: modulated delay line with configurable LFO, stereo offset, and mix.

  See also:
    alternative PhaseFX - allpass filter sweep rather than delay-line pitch modulation
    scriptnode jdsp.jchorus - LFO-modulated delay chorus with configurable mix
---

::category-tags
---
tags:
  - { name: delay, desc: "Effects based on delayed signal copies, including chorus and phaser" }
---
::

![Chorus screenshot](/images/v2/reference/audio-modules/chorus.png)

A simple stereo chorus effect that uses an internal parabolic LFO to modulate a delay line. The modulated delay creates the characteristic pitch variation and thickening associated with chorus effects. The wet signal is subtractively mixed with the dry signal at a fixed ratio that cannot be adjusted.

The Chorus is lightweight and straightforward but has limited control compared to a scriptnode equivalent. Both channels share the same LFO phase (no stereo offset), the wet/dry mix is fixed, and the Feedback parameter uses an unusual mapping where 0.5 equals zero feedback.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Rate:
      desc: "Internal LFO speed (approximately 0.01-10 Hz, logarithmic)"
      range: "0 - 100%"
      default: "30%"
    Width:
      desc: "Modulation depth controlling pitch variation intensity"
      range: "0 - 100%"
      default: "43%"
    Feedback:
      desc: "Delay line feedback (-0.95 to +0.95 internally)"
      range: "0 - 100%"
      default: "30%"
    Delay:
      desc: "Shifts the modulation centre point"
      range: "0 - 100%"
      default: "100%"
  functions:
    readInterpolated:
      desc: "Reads from the delay buffer at the LFO-modulated position using linear interpolation between adjacent samples"
---

```
// Chorus - monophonic, fixed 53/47 dry/wet mix
// stereo in -> stereo out

process(left, right) {
    // Internal parabolic LFO (shared for both channels)
    lfoPhase += Rate
    lfoValue = 1 - lfoPhase * lfoPhase

    // Modulated delay position
    delayPos = Delay.offset + Width.depth * lfoValue

    // Write input + feedback to circular buffer
    buffer[writePos] = sample + Feedback * lastDelayed

    // Read from modulated position
    delayed = readInterpolated(buffer, delayPos)

    // Fixed subtractive mix (not adjustable)
    output = 0.53 * sample - 0.47 * delayed
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Modulation
    params:
      - { name: Rate, desc: "Controls the speed of the internal LFO. Maps logarithmically to approximately 0.01-10 Hz. Below 1%, the LFO is frozen and the chorus becomes a static delay.", range: "0 - 100%", default: "30%" }
      - { name: Width, desc: "Controls the depth of the delay modulation, which determines the intensity of the pitch variation. Higher values create a more pronounced chorus effect. The maximum modulation depth is approximately 45ms at 44.1kHz.", range: "0 - 100%", default: "43%" }
  - label: Delay
    params:
      - { name: Feedback, desc: "Controls the amount of delayed signal fed back into the buffer. Maps to an internal range of -0.95 to +0.95: values below 50% produce negative feedback, 50% is zero feedback, and values above 50% produce positive feedback. The default of 30% gives a negative feedback of -0.38.", range: "0 - 100%", default: "30%" }
      - { name: Delay, desc: "Shifts the centre point of the delay modulation. At 100% (default), the modulation sweeps from zero to the maximum depth set by Width. Lower values offset the modulation range upward, increasing the minimum delay time.", range: "0 - 100%", default: "100%" }
---
::

### Limitations

The wet/dry mix is hardcoded at approximately 47% wet. There is no Mix parameter to adjust this ratio. To blend the effect, use a send/return routing. [1]($FORUM_REF.4525$)

Both channels share the same internal LFO phase. Any stereo effect comes only from different feedback histories accumulating in each channel's delay buffer, not from LFO phase offset. For wider stereo, build a custom chorus in scriptnode with independent L/R LFO phases. [2]($FORUM_REF.4525$)

The wet signal is subtracted from the dry signal, not added. This creates a comb-filter cancellation pattern characteristic of the mda chorus algorithm on which this module is based. [3]($FORUM_REF.980$)

The delay buffer holds 2048 samples. At 44.1 kHz, the maximum modulation depth is approximately 45 ms. For more flexible chorus effects, the scriptnode `jchorus` node is recommended. [4]($FORUM_REF.12406$)

**See also:** $MODULES.PhaseFX$ -- Uses allpass filters for frequency notch sweeping rather than delay-line pitch modulation, $SN.jdsp.jchorus$ -- Scriptnode LFO-modulated delay chorus with configurable mix and parameters
