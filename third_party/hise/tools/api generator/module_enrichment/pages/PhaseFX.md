---
title: Phase FX
moduleId: PhaseFX
type: Effect
subtype: MasterEffect
tags: [delay]
builderPath: b.Effects.PhaseFX
screenshot: /images/v2/reference/audio-modules/phasefx.png
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: Chorus, type: alternative, reason: "Uses modulated delay lines rather than allpass filters, producing pitch-shift chorus rather than notch sweep" }
  - { id: "fx.phase_delay", type: scriptnode, reason: "Scriptnode allpass phase-shift building block (PhaseFX uses cascaded allpass stages)" }
commonMistakes:
  - title: "Requires external modulation"
    wrong: "Adding a Phase FX and expecting it to sweep automatically"
    right: "Add an LFO or other modulator to the Phase Modulation chain to create sweep movement"
    explanation: "Phase FX has no internal LFO. Without a modulator in the Phase Modulation chain, the phaser produces a static frequency notch pattern with no sweep."
  - title: "Mix defaults to 100% wet"
    wrong: "Setting Mix to 0.5 expecting a subtle effect"
    right: "Mix defaults to 1.0 (fully wet). The phaser output already contains the dry signal summed with the allpass output, so Mix controls the blend between unprocessed and phased signal."
    explanation: "At Mix=1.0 the output is the full phaser signal (input + allpass output). Reducing Mix blends toward the unprocessed input."
forumReferences:
  - id: 1
    title: "All 6 allpass stages share the same frequency — Frequency1/2 are sweep range bounds"
    summary: "Frequency1 and Frequency2 define the lower and upper bounds of the sweep range; all 6 stages are always set to the same frequency coefficient derived from the modulation position."
    topic: 12772
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: medium
  description: "Chain of allpass nodes with feedback and modulated delay coefficient in a scriptnode network"
llmRef: |
  Phase FX (MasterEffect)

  Phaser effect using 6 cascaded allpass filters with feedback to create sweeping frequency notches. Requires external modulation (e.g. LFO) in the Phase Modulation chain to produce sweep movement - has no internal LFO.

  Signal flow:
    audio in -> [input + feedback * prevOutput] -> 6x allpass cascade -> sum with input -> dry/wet mix -> audio out

  CPU: medium, monophonic (MasterEffect).

  Parameters:
    Frequency1 (20-20000 Hz, default 400 Hz) - lower bound of the sweep range
    Frequency2 (20-20000 Hz, default 1600 Hz) - upper bound of the sweep range
    Feedback (0-100%, default 70%) - resonance intensity around the allpass cascade. Scaled by 0.99 for safety.
    Mix (0-100%, default 100%) - linear dry/wet crossfade

  Modulation chains:
    Phase Modulation - sweeps allpass frequencies between Frequency1 and Frequency2. Audio-rate capable. 0 = Frequency1, 1 = Frequency2.

  When to use:
    Classic phaser sweep effects. Add an LFO to the Phase Modulation chain for auto-sweep, or use an envelope for dynamic phasing.

  Common mistakes:
    No internal LFO - must add a modulator to the Phase Modulation chain for sweep.
    Mix defaults to 100% wet.

  Custom equivalent:
    scriptnode HardcodedFX: chain of allpass nodes with feedback and modulated delay.

  See also:
    alternative Chorus - modulated delay lines instead of allpass filters
    [scriptnode] fx.phase_delay - scriptnode allpass phase-shift building block (PhaseFX uses cascaded allpass stages)
---

::category-tags
---
tags:
  - { name: delay, desc: "Effects based on delayed signal copies, including chorus and phaser" }
---
::

![Phase FX screenshot](/images/v2/reference/audio-modules/phasefx.png)

Phase FX creates sweeping frequency notches by passing the signal through a cascade of six allpass filters with feedback. The sweep position is controlled entirely by the Phase Modulation chain - there is no internal LFO, so you must add a modulator (typically an LFO) to create the characteristic phaser sweep.

The two Frequency parameters define the sweep range: the modulation chain interpolates the allpass filter frequencies between Frequency1 and Frequency2. Feedback controls how pronounced the notches are, creating stronger resonance at higher values.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency1:
      desc: "Lower bound of the phaser sweep range"
      range: "20 - 20000 Hz"
      default: "400 Hz"
    Frequency2:
      desc: "Upper bound of the phaser sweep range"
      range: "20 - 20000 Hz"
      default: "1600 Hz"
    Feedback:
      desc: "Resonance intensity around the allpass cascade"
      range: "0 - 100%"
      default: "70%"
    Mix:
      desc: "Linear dry/wet crossfade"
      range: "0 - 100%"
      default: "100%"
  functions:
    allpassCascade:
      desc: "Six first-order allpass filters in series, all sharing the same delay coefficient derived from the modulation position"
  modulations:
    PhaseModulation:
      desc: "Sweeps the allpass frequencies between Frequency1 and Frequency2"
      scope: "monophonic"
---

```
// Phase FX - monophonic phaser
// stereo in -> stereo out

process(left, right) {
    // Sweep position from modulation chain (audio-rate)
    freq = Frequency1 + (Frequency2 - Frequency1) * PhaseModulation

    // Allpass cascade with feedback (per sample, per channel)
    input = sample + prevOutput * Feedback
    output = allpassCascade(input, freq)    // 6 allpass stages
    prevOutput = output

    // Phaser output = original + allpass (creates notch pattern)
    phased = sample + output

    // Dry/wet mix
    sample = sample * (1 - Mix) + phased * Mix
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Sweep Range
    params:
      - { name: Frequency1, desc: "Lower frequency bound of the phaser sweep. The Phase Modulation chain interpolates between this and Frequency2.", range: "20 - 20000 Hz", default: "400 Hz" }
      - { name: Frequency2, desc: "Upper frequency bound of the phaser sweep. The Phase Modulation chain interpolates between Frequency1 and this.", range: "20 - 20000 Hz", default: "1600 Hz" }
  - label: Effect
    params:
      - { name: Feedback, desc: "Controls the resonance of the phaser notches. Higher values create more pronounced, ringing notches. Internally scaled by 0.99 to prevent instability.", range: "0 - 100%", default: "70%" }
      - { name: Mix, desc: "Dry/wet balance. At 100% (default), the output is the full phaser signal. At 0%, the signal passes through unaffected.", range: "0 - 100%", default: "100%" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Phase Modulation", desc: "Controls the sweep position between Frequency1 and Frequency2. At 0, the allpass filters are tuned to Frequency1; at 1, they are tuned to Frequency2. Expanded to audio rate for sample-accurate sweep control.", scope: "monophonic", constrainer: "Any" }
---
::

### Sweep Setup

Phase FX has no internal LFO. Without modulators in the Phase Modulation chain, the phaser produces a static notch pattern. To create the classic phaser sweep, add an LFO modulator to the Phase Modulation chain. Frequency1 and Frequency2 define the lower and upper bounds of the sweep range that the modulation interpolates between - all 6 allpass stages share the same frequency at any given sample. [1]($FORUM_REF.12772$)

Frequency1 and Frequency2 are smoothed with a 50 ms ramp to prevent clicks when changed via automation or scripting.

**See also:** $MODULES.Chorus$ -- Uses modulated delay lines rather than allpass filters, producing pitch-shift chorus rather than notch sweep, $SN.fx.phase_delay$ -- scriptnode allpass phase-shift building block (PhaseFX uses cascaded allpass stages)
