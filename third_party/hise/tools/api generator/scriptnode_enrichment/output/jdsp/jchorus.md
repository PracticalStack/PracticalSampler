---
title: Chorus
description: "A chorus effect with LFO-modulated delay, feedback, and wet/dry mixing."
factoryPath: jdsp.jchorus
factory: jdsp
polyphonic: false
tags: [jdsp, modulation-effect, chorus]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "jdsp.jdelay", type: companion, reason: "Raw delay line for building custom modulation effects" }
  - { id: "jdsp.jdelay_cubic", type: companion, reason: "Delay with flat frequency response for cleaner chorus/flanger" }
  - { id: "Chorus", type: module, reason: "Direct equivalent -- LFO-modulated delay chorus" }
commonMistakes:
  - title: "CentreDelay near maximum causes clipping"
    wrong: "Setting CentreDelay to exactly 100 ms"
    right: "Keep CentreDelay below 100 ms. Values are internally clamped to 99.9 ms."
    explanation: "The CentreDelay is clamped to just under 100 ms to avoid edge cases in the internal delay buffer. Setting it to the maximum has no effect beyond 99.9 ms."
llmRef: |
  jdsp.jchorus

  A chorus effect that modulates a delay line with an internal LFO. Produces the characteristic shimmering, widened sound of classic chorus effects. Monophonic -- processes all voices through a single shared effect.

  Signal flow:
    audio in -> LFO-modulated delay + feedback -> dry/wet mix -> audio out

  CPU: medium, monophonic

  Parameters:
    CentreDelay (0 - 100 ms, default 7.0) - base delay time around which the LFO modulates
    Depth (0 - 100%, default 25%) - LFO modulation depth
    Feedback (-100% to 100%, default 0%) - feedback amount; negative values invert the delayed signal phase
    Rate (0 - 100 Hz, default 1.0 Hz, log skew) - LFO rate
    Mix (0 - 100%, default 50%) - dry/wet balance

  When to use:
    Stereo widening, thickening, or classic chorus/flanger effects. For more control over the delay modulation, build a custom chorus using jdelay or jdelay_cubic with an external LFO.

  Common mistakes:
    CentreDelay near maximum is clamped to 99.9 ms.

  See also:
    companion jdsp.jdelay -- raw delay line for custom modulation effects
    companion jdsp.jdelay_cubic -- delay with flat frequency response
    [module] Chorus -- module-tree LFO-modulated delay chorus
---

A chorus effect that modulates a delay line with an internal LFO. The delayed copies of the input signal are pitch-shifted slightly by the LFO modulation, creating a thicker, wider sound. Adjusting Rate and Depth controls the speed and intensity of the modulation, whilst CentreDelay sets the base delay time that determines the overall character.

Feedback feeds the delayed signal back into the delay line. Positive values reinforce the delay, creating a more resonant, flanging-type effect. Negative values invert the phase of the delayed signal before feeding it back, producing a different tonal character. The Mix parameter controls the balance between the original dry signal and the processed wet signal.

## Signal Path

::signal-path
---
glossary:
  parameters:
    CentreDelay:
      desc: "Base delay time for the modulated delay line"
      range: "0 - 100 ms"
      default: "7.0"
    Depth:
      desc: "LFO modulation depth"
      range: "0 - 100%"
      default: "25%"
    Feedback:
      desc: "Feedback amount; negative values invert phase"
      range: "-100% - 100%"
      default: "0%"
    Rate:
      desc: "LFO modulation rate"
      range: "0 - 100 Hz"
      default: "1.0"
    Mix:
      desc: "Dry/wet balance"
      range: "0 - 100%"
      default: "50%"
---

```
// jdsp.jchorus - LFO-modulated delay chorus
// audio in -> audio out

process(input) {
    lfo = oscillate(Rate, Depth)
    delayed = delay(input, CentreDelay + lfo)
    delayed += delayed * Feedback
    output = input * (1 - Mix) + delayed * Mix
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Modulation
    params:
      - { name: Rate, desc: "LFO modulation rate. Higher values produce faster pitch wobble; lower values produce a slow, gentle movement.", range: "0 - 100 Hz", default: "1.0" }
      - { name: Depth, desc: "LFO modulation depth. Controls how far the delay time swings around the CentreDelay value.", range: "0 - 100%", default: "25%" }
  - label: Delay
    params:
      - { name: CentreDelay, desc: "Base delay time around which the LFO modulates. Shorter values produce a tighter, flanger-like effect; longer values produce a more pronounced chorus.", range: "0 - 100 ms", default: "7.0" }
  - label: Character
    params:
      - { name: Feedback, desc: "Feeds the delayed signal back into the delay line. Negative values invert the phase of the feedback signal, producing a different harmonic character.", range: "-100% - 100%", default: "0%" }
      - { name: Mix, desc: "Balance between the dry input signal and the wet chorus signal.", range: "0 - 100%", default: "50%" }
---
::

**See also:** [$SN.jdsp.jdelay$]($SN.jdsp.jdelay$) -- raw delay line for building custom modulation effects, [$SN.jdsp.jdelay_cubic$]($SN.jdsp.jdelay_cubic$) -- delay with flat frequency response for cleaner chorus/flanger, $MODULES.Chorus$ -- module-tree LFO-modulated delay chorus
