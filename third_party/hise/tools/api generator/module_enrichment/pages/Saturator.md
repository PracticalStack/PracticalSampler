---
title: Saturator
moduleId: Saturator
type: Effect
subtype: MasterEffect
tags: [dynamics]
builderPath: b.Effects.Saturator
screenshot: /images/v2/reference/audio-modules/saturator.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Saturation needs PreGain to be audible"
    wrong: "Adding saturation without adjusting PreGain and expecting a noticeable effect"
    right: "Use PreGain to drive the signal into the waveshaper and PostGain to compensate the output level"
    explanation: "The waveshaping curve is amplitude-dependent. Quiet signals pass through almost unchanged regardless of the Saturation setting. Boost with PreGain to push the signal into the nonlinear range, then reduce with PostGain to restore the original level."
  - title: "Saturation=0 still applies gain staging"
    wrong: "Setting Saturation to 0 and expecting the module to be a transparent bypass"
    right: "At Saturation=0 the waveshaper is linear, but PreGain and PostGain still apply"
    explanation: "Even with no saturation, the gain staging parameters are active. Set PreGain to 0 dB and PostGain to 0 dB for true passthrough."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: simple
  description: "A gain node, waveshaper node, and mix node in a scriptnode network provide the same chain with more waveshaping options"
llmRef: |
  Saturator (MasterEffect)

  Simple soft-clip waveshaper with pre/post gain staging and dry/wet mix. Uses the formula (1+k)*x / (1+k*|x|) where k = 2*saturation/(1-saturation). No oversampling. Saturation Modulation chain scales the saturation amount.

  Signal flow:
    audio in -> PreGain (0-24 dB boost) -> soft-clip waveshaper -> PostGain (-24 to 0 dB cut) -> dry/wet mix -> audio out

  CPU: low, monophonic (MasterEffect).

  Parameters:
    Saturation (0-100%, default 0%) - waveshaping intensity. 0 = linear, approaching 100% = hard clip.
    WetAmount (0-100%, default 100%) - linear dry/wet crossfade
    PreGain (0-24 dB, default 0 dB) - boost-only input gain before saturation
    PostGain (-24 to 0 dB, default 0 dB) - cut-only output gain after saturation

  Modulation chains:
    Saturation Modulation - scales the saturation amount. Updated every 8 samples when audio-rate modulation is active.

  When to use:
    Adding warmth, harmonics, or soft clipping to a signal. Use PreGain to drive into the nonlinear range and PostGain to compensate.

  Common mistakes:
    Signal must be driven with PreGain to hear saturation on quiet signals.
    At Saturation=0 the waveshaper is linear but gain staging still applies.

  Custom equivalent:
    scriptnode HardcodedFX: gain + waveshaper + mix nodes.

  See also:
    (none)
---

::category-tags
---
tags:
  - { name: dynamics, desc: "Effects that shape the amplitude or add distortion and saturation" }
---
::

![Saturator screenshot](/images/v2/reference/audio-modules/saturator.png)

A simple soft-clip waveshaper that adds harmonic saturation to the signal. The effect uses a smooth transfer function that progressively limits signal amplitude as the Saturation parameter increases, introducing odd harmonics without hard clipping. Pre and post gain controls allow driving the signal into the nonlinear range and compensating the output level.

The Saturation Modulation chain allows dynamic control of the saturation intensity from envelopes, LFOs, or other modulators. At Saturation=0 the waveshaper is fully linear, making the module behave as a pure gain stage with just PreGain and PostGain.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Saturation:
      desc: "Waveshaping intensity, from linear (0%) to near-hard-clip (100%)"
      range: "0 - 100%"
      default: "0%"
    PreGain:
      desc: "Boost-only input gain applied before saturation"
      range: "0 - 24 dB"
      default: "0 dB"
    PostGain:
      desc: "Cut-only output gain applied after saturation"
      range: "-24 - 0 dB"
      default: "0 dB"
    WetAmount:
      desc: "Linear dry/wet crossfade"
      range: "0 - 100%"
      default: "100%"
  functions:
    softClip:
      desc: "Soft-clip transfer function: (1+k)*x / (1+k*|x|), producing smooth amplitude limiting and odd harmonics"
  modulations:
    SaturationModulation:
      desc: "Scales the saturation amount"
      scope: "monophonic"
---

```
// Saturator - monophonic soft-clip waveshaper
// stereo in -> stereo out

process(left, right) {
    // Saturation amount (modulated)
    sat = Saturation * SaturationModulation

    // Per-sample processing chain
    driven = sample * PreGain
    saturated = softClip(driven, sat)
    gained = saturated * PostGain

    // Dry/wet mix
    output = (1 - WetAmount) * sample + WetAmount * gained
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Saturation
    params:
      - name: Saturation
        desc: "Controls the intensity of the waveshaping. At 0% the signal passes through linearly. As the value increases, the transfer function progressively limits signal peaks, adding odd harmonics. Near 100% the curve approaches hard clipping."
        range: "0 - 100%"
        default: "0%"
        hints:
          - type: warning
            text: "No oversampling is applied. At high saturation with high-frequency content, aliasing may be audible. For critical applications, use $MODULES.ShapeFX$ which offers oversampling options."
  - label: Gain Staging
    params:
      - { name: PreGain, desc: "Boost-only input gain applied before the waveshaper. Use this to drive the signal into the nonlinear range. At 0 dB the signal is unchanged.", range: "0 - 24 dB", default: "0 dB" }
      - { name: PostGain, desc: "Cut-only output gain applied after the waveshaper. Use this to compensate for the level increase caused by PreGain and saturation. At 0 dB the signal is unchanged.", range: "-24 - 0 dB", default: "0 dB" }
  - label: Mix
    params:
      - { name: WetAmount, desc: "Linear crossfade between the original dry signal and the saturated signal. At 100% (default) the output is fully saturated. At 0% the original signal passes through.", range: "0 - 100%", default: "100%" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Saturation Modulation", desc: "Scales the Saturation parameter value. The modulation output multiplies the parameter value rather than replacing it. When audio-rate modulators are used, the saturation amount is updated every 8 samples.", scope: "monophonic", constrainer: "TimeVariantModulator" }
---
::

## See Also

::see-also
---
links: []
---
::
