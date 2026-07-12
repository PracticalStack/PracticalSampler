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
    explanation: "CentreDelay clamped to just under 100 ms to avoid edge cases in internal delay buffer. Setting to maximum has no effect beyond 99.9 ms."
llmRef: |
  jdsp.jchorus

  Chorus effect modulating a delay line with internal LFO. Produces characteristic shimmering, widened sound of classic chorus effects. Monophonic -- processes all voices through single shared effect.

  Signal flow:
    audio in -> LFO-modulated delay + feedback -> dry/wet mix -> audio out

  CPU: medium, monophonic

  Parameters:
    CentreDelay (0 - 100 ms, default 7.0) - base delay time around which LFO modulates
    Depth (0 - 100%, default 25%) - LFO modulation depth
    Feedback (-100% to 100%, default 0%) - feedback amount; negative values invert delayed signal phase
    Rate (0 - 100 Hz, default 1.0 Hz, log skew) - LFO rate
    Mix (0 - 100%, default 50%) - dry/wet balance

  When to use:
    Stereo widening, thickening, or classic chorus/flanger effects. For more control over delay modulation, build custom chorus using jdelay or jdelay_cubic with external LFO.

  Common mistakes:
    CentreDelay near maximum is clamped to 99.9 ms.

  See also:
    companion jdsp.jdelay -- raw delay line for custom modulation effects
    companion jdsp.jdelay_cubic -- delay with flat frequency response
    [module] Chorus -- module-tree LFO-modulated delay chorus
---

Chorus effect modulating a delay line with internal LFO. Delayed copies of input signal are pitch-shifted slightly by LFO modulation, creating thicker, wider sound. Rate and Depth controls adjust modulation speed and intensity; CentreDelay sets base delay time determining overall character.

Feedback feeds delayed signal back into delay line. Positive values reinforce delay, creating resonant, flanging-type effect. Negative values invert phase of delayed signal before feeding back, producing different tonal character. Mix parameter controls balance between original dry signal and processed wet signal.

## Signal Path

::signal-path
---
glossary:
  parameters:
    CentreDelay:
      desc: "Base delay time for modulated delay line"
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
      - { name: Rate, desc: "LFO modulation rate. Higher values produce faster pitch wobble; lower values produce slow, gentle movement.", range: "0 - 100 Hz", default: "1.0" }
      - { name: Depth, desc: "LFO modulation depth. Controls how far delay time swings around CentreDelay value.", range: "0 - 100%", default: "25%" }
  - label: Delay
    params:
      - { name: CentreDelay, desc: "Base delay time around which LFO modulates. Shorter values produce tighter, flanger-like effect; longer values produce more pronounced chorus.", range: "0 - 100 ms", default: "7.0" }
  - label: Character
    params:
      - { name: Feedback, desc: "Feeds delayed signal back into delay line. Negative values invert phase of feedback signal, producing different harmonic character.", range: "-100% - 100%", default: "0%" }
      - { name: Mix, desc: "Balance between dry input signal and wet chorus signal.", range: "0 - 100%", default: "50%" }
---
::

**See also:** [$SN.jdsp.jdelay$]($SN.jdsp.jdelay$) -- raw delay line for building custom modulation effects, [$SN.jdsp.jdelay_cubic$]($SN.jdsp.jdelay_cubic$) -- delay with flat frequency response for cleaner chorus/flanger, $MODULES.Chorus$ -- module-tree LFO-modulated delay chorus
