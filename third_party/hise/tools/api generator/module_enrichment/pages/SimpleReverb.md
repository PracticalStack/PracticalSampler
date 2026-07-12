---
title: Simple Reverb
moduleId: SimpleReverb
type: Effect
subtype: MasterEffect
tags: [reverb]
builderPath: b.Effects.SimpleReverb
screenshot: /images/v2/reference/audio-modules/simplereverb.png
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "fx.reverb", type: scriptnode, reason: "Direct equivalent -- Freeverb algorithm" }
commonMistakes:
  - title: "Dry Level is automatically controlled"
    wrong: "Adjusting the Dry Level slider expecting it to independently control the dry signal"
    right: "Only the Wet Level slider controls the mix. Dry Level is automatically set to 1 minus Wet Level."
    explanation: "The Dry Level parameter appears in the interface but has no independent effect. Changing Wet Level sets both wet and dry levels."
  - title: "FreezeMode is threshold toggle at 50%"
    wrong: "Setting Freeze Mode to a value like 0.3 expecting partial freezing"
    right: "Freeze Mode acts as a threshold toggle at 0.5. Below 0.5 it has no effect; at 0.5 or above it fully freezes the reverb tail."
    explanation: "Despite the continuous 0-1 range, the parameter behaves as an on/off switch with the threshold at 0.5."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: medium
  description: "The fx.reverb scriptnode node wraps the same Freeverb algorithm with additional routing flexibility"
llmRef: |
  Simple Reverb (MasterEffect)

  Algorithmic reverb based on the Freeverb algorithm (JUCE Reverb class). Provides basic room simulation with controls for room size, damping, wet level, stereo width, and freeze mode. A fixed 0.5x gain compensation is applied after processing. The Dry Level parameter is vestigial - it is automatically slaved to 1 minus Wet Level.

  Signal flow:
    audio in -> Freeverb (8 comb + 4 allpass per channel, stereo decorrelation, internal wet/dry mix) -> 0.5x gain -> audio out

  CPU: medium, monophonic (MasterEffect).

  Parameters:
    RoomSize (0-100%, default 80%) - perceived room size
    Damping (0-100%, default 60%) - high-frequency absorption in the reverb tail
    WetLevel (0-100%, default 20%) - controls both wet and dry levels (dry = 1 - wet)
    DryLevel (0-100%, default 80%) - has no independent effect (vestigial, slaved to WetLevel)
    Width (0-100%, default 80%) - stereo width of the reverb output
    FreezeMode (0-100%, default 10%) - threshold toggle at 50%: below = off, above = infinite sustain

  When to use:
    Quick, lightweight reverb for basic spatial effects. For higher quality reverb, consider convolution or third-party options.

  Common mistakes:
    DryLevel slider has no independent effect.
    FreezeMode is a threshold toggle at 0.5, not a continuous control.

  Custom equivalent:
    scriptnode HardcodedFX: fx.reverb node (same Freeverb algorithm).

  See also:
    [scriptnode] fx.reverb - direct equivalent -- Freeverb algorithm
---

::category-tags
---
tags:
  - { name: reverb, desc: "Effects that simulate room acoustics and spatial reflections" }
---
::

![Simple Reverb screenshot](/images/v2/reference/audio-modules/simplereverb.png)

A lightweight algorithmic reverb based on the Freeverb algorithm. It provides basic room simulation with controls for room size, high-frequency damping, stereo width, and a freeze mode that sustains the reverb tail indefinitely. The reverb quality is modest compared to convolution-based alternatives but is useful for simple spatial effects with low CPU usage.

The Wet Level parameter controls the wet/dry balance directly - the Dry Level parameter is shown in the interface but has no independent effect (it is automatically set to 1 minus Wet Level). A fixed gain reduction is applied after processing to compensate for the reverb's tendency to increase overall level.

## Signal Path

::signal-path
---
glossary:
  parameters:
    RoomSize:
      desc: "Perceived room size controlling reverb decay length"
      range: "0 - 100%"
      default: "80%"
    Damping:
      desc: "High-frequency absorption in the reverb tail"
      range: "0 - 100%"
      default: "60%"
    WetLevel:
      desc: "Controls both wet and dry levels (dry = 1 - wet)"
      range: "0 - 100%"
      default: "20%"
    Width:
      desc: "Stereo width of the reverb output"
      range: "0 - 100%"
      default: "80%"
    FreezeMode:
      desc: "Threshold toggle: below 50% = off, at/above 50% = infinite sustain"
      range: "0 - 100%"
      default: "10%"
  functions:
    freeverb:
      desc: "Freeverb algorithm: 8 parallel comb filters into 4 series allpass filters per channel with stereo decorrelation, internal wet/dry mixing, and width control"
---

```
// Simple Reverb - monophonic Freeverb wrapper
// stereo in -> stereo out

process(left, right) {
    // Freeverb algorithm (handles wet/dry, width, damping internally)
    [left, right] = freeverb(left, right, RoomSize, Damping, WetLevel, Width, FreezeMode)

    // Fixed gain compensation
    left  *= 0.5
    right *= 0.5
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Reverb Character
    params:
      - { name: RoomSize, desc: "Controls the perceived room size, which affects the length of the reverb tail. Low values produce a small, tight space; high values produce a large, open hall.", range: "0 - 100%", default: "80%" }
      - { name: Damping, desc: "Controls high-frequency absorption in the reverb tail. Higher values create a darker, warmer reverb as high frequencies decay faster. Lower values produce a brighter tail.", range: "0 - 100%", default: "60%" }
  - label: Mix
    params:
      - { name: WetLevel, desc: "Controls the wet/dry balance. Also automatically sets the dry level to 1 minus this value. At 0% the output is fully dry; at 100% fully wet.", range: "0 - 100%", default: "20%" }
      - { name: DryLevel, desc: "This parameter has no independent effect. It is automatically set to 1 minus Wet Level.", range: "0 - 100%", default: "80%" }
  - label: Stereo
    params:
      - { name: Width, desc: "Controls the stereo spread of the reverb output. At 0% the reverb is mono; at 100% the full stereo decorrelation is used.", range: "0 - 100%", default: "80%" }
  - label: Special
    params:
      - { name: FreezeMode, desc: "Sustains the reverb tail indefinitely when the value is at or above 50%. Below 50% it has no effect. Despite the continuous slider, it behaves as an on/off toggle at the 50% threshold.", range: "0 - 100%", default: "10%" }
---
::

### Wet/Dry Balance

The Wet Level parameter controls both the wet and dry signal levels: setting Wet Level to X automatically sets Dry Level to 1 minus X. The Dry Level parameter appears in the interface but changing it has no effect.

A fixed 0.5x gain reduction is applied to the output after the reverb algorithm. This compensates for the Freeverb algorithm's tendency to increase overall signal level, particularly at high room sizes. The reverb tail is cleared when voices are killed, preventing lingering reverb from previous notes.

**See also:** $SN.fx.reverb$ -- direct equivalent -- Freeverb algorithm
