---
title: Stereo FX
moduleId: StereoFX
type: Effect
subtype: VoiceEffect
tags: [mixing]
builderPath: b.Effects.StereoFX
screenshot: /images/v2/reference/audio-modules/stereofx.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: PolyshapeFX, type: alternative, reason: "Another polyphonic effect - applies waveshaping per-voice rather than stereo processing" }
  - { id: SimpleGain, type: disambiguation, reason: "Monophonic gain and balance control. Use SimpleGain for static stereo balance; use StereoFX for per-voice modulated panning" }
  - { id: "jdsp.jpanner", type: scriptnode, reason: "Scriptnode stereo panner with seven selectable pan laws" }
commonMistakes:
  - title: "Pan needs modulators to work"
    wrong: "Setting the Pan parameter and expecting a static pan position"
    right: "Add modulators to the Pan Modulation chain to hear the Pan parameter's effect"
    explanation: "The Pan parameter defines the maximum modulation range, not a static pan position. Without modulators in the Pan Modulation chain, the Pan parameter has no audible effect."
  - title: "Width has no effect on mono"
    wrong: "Using Width to add stereo spread to a mono signal"
    right: "Width only affects signals that already have stereo content"
    explanation: "Width uses mid/side processing. If left and right channels are identical (mono), the side signal is zero and Width has no effect regardless of its setting."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: simple
  description: "A scriptnode network with a pan node and mid/side width node can replicate this module"
llmRef: |
  StereoFX (VoiceEffect, polyphonic)

  Polyphonic stereo panner with mid/side width control. Pan is modulation-driven and applied per-voice; Width is applied once to the combined output.

  Signal flow:
    per-voice: audio in -> pan modulation (equal-power, audio-rate capable) -> panned voice output
    shared: combined voices -> mid/side width processing -> audio out

  CPU: low, polyphonic (scales with voice count for pan; width runs once on combined output).

  Parameters:
    Pan (-100 to 100, default 100) - maximum pan modulation range. Not a static position. Requires modulators in the Pan Modulation chain.
    Width (0 to 200%, default 100%) - stereo width via mid/side encoding. 0=mono, 100=passthrough, 200=exaggerated stereo.

  Modulation chains:
    Pan Modulation - drives the per-voice pan position. Audio-rate capable. Accepts any modulator type.

  When to use:
    Per-voice stereo placement driven by modulators (LFOs, envelopes, random). Not a static pan knob - use SimpleGain for static balance.

  Common mistakes:
    Pan parameter does nothing without modulators in the Pan Modulation chain.
    Width has no effect on mono signals (identical L/R).

  Custom equivalent:
    scriptnode HardcodedFX: pan + mid/side width nodes.

  See also:
    PolyshapeFX - another polyphonic effect (waveshaping per-voice)
    SimpleGain - monophonic gain/balance for static stereo positioning
    scriptnode jdsp.jpanner - stereo panner with seven selectable pan laws
---

::category-tags
---
tags:
  - { name: mixing, desc: "Effects that control volume, stereo width, or stereo balance" }
---
::

![Stereo FX screenshot](/images/v2/reference/audio-modules/stereofx.png)

A polyphonic stereo processor with two stages: per-voice panning and shared stereo width. The Pan parameter defines the maximum modulation range for the Pan Modulation chain rather than setting a static pan position - without modulators in the chain, no panning occurs. Each voice is panned independently using an equal-power pan law, allowing per-voice stereo placement driven by LFOs, envelopes, or random modulators. The pan modulation supports audio-rate updates for smooth, zipper-free sweeps.

The Width parameter applies mid/side processing to the combined output after all voices are summed. At 0% the output collapses to mono, at 100% (default) the signal passes through unchanged, and values above 100% exaggerate the stereo difference. Because Width operates on the summed buffer, it affects all voices equally.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Pan:
      desc: "Maximum pan modulation range (-100 to +100). Scales the Pan Modulation output."
      range: "-100 - 100"
      default: "100"
    Width:
      desc: "Stereo width via mid/side processing (0=mono, 100=passthrough, 200=exaggerated)"
      range: "0 - 200%"
      default: "100%"
  functions:
    equalPowerPan:
      desc: "Equal-power pan law: L = sqrt(2) * cos(angle), R = sqrt(2) * sin(angle). Unity gain at centre."
    midSideWidth:
      desc: "Mid/side encode, scale side signal by width factor, decode back to L/R"
  modulations:
    PanModulation:
      desc: "Drives the per-voice pan position. Audio-rate capable."
      scope: "per-voice"
---

```
// Stereo FX - polyphonic panner + stereo width
// stereo in (per-voice) -> stereo out

// Stage 1: Per-voice panning
applyToVoice(left, right) {
    if (PanModulation chain is empty)
        return  // Pan parameter has no effect without modulators

    // Scale modulation by parameter range
    scaledPan = PanModulation * Pan

    // Apply equal-power pan law to voice buffer
    left, right = equalPowerPan(left, right, scaledPan)
}

// Stage 2: Shared width (runs once on combined output)
processOutput(left, right) {
    if (Width == 100%)
        return  // passthrough

    left, right = midSideWidth(left, right, Width)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Pan
    params:
      - { name: Pan, desc: "Defines the maximum range of the Pan Modulation chain. At 100 (default), modulators can sweep the full stereo field. At 50, the sweep is limited to half the field. This parameter has no audible effect without modulators in the Pan Modulation chain.", range: "-100 - 100", default: "100" }
  - label: Width
    params:
      - { name: Width, desc: "Controls the stereo width of the combined output using mid/side processing. At 0% the output is mono. At 100% the signal passes through unchanged. Above 100% the stereo difference is exaggerated, which can produce out-of-phase content at extreme settings. Has no effect on mono signals.", range: "0 - 200%", default: "100%" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Pan Modulation", desc: "Drives the per-voice pan position. The modulation output is scaled by the Pan parameter to determine the final stereo placement of each voice. Supports audio-rate modulation for smooth pan sweeps. Monophonic modulators are included in per-voice rendering.", scope: "per-voice", constrainer: "*" }
---
::

### Pan Behaviour

The Pan parameter is a modulation range scaler, not a static pan control. Without a modulator in the Pan Modulation chain, the Pan knob has no audible effect. [1]($FORUM_REF.1081$) For static stereo balance, use $MODULES.SimpleGain$ instead. Pan uses an equal-power pan law that maintains unity gain at centre.

### Width Behaviour

Width is applied to the combined output buffer after all voices are summed, not per-voice. Width has no effect on a mono source (where L equals R) because the side signal is zero. [2]($FORUM_REF.7956$) The Width parameter has no modulation chain - for dynamic width control, build a mid/side network in scriptnode. [3]($FORUM_REF.6144$)

**See also:** $MODULES.PolyshapeFX$ -- Another polyphonic effect - applies waveshaping per-voice rather than stereo processing, $MODULES.SimpleGain$ -- Monophonic gain and balance control. Use SimpleGain for static stereo balance; use StereoFX for per-voice modulated panning, $SN.jdsp.jpanner$ -- Scriptnode stereo panner with seven selectable pan laws
