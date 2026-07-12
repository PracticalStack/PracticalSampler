---
title: Simple Gain
moduleId: SimpleGain
type: Effect
subtype: MasterEffect
tags: [mixing, utility]
builderPath: b.Effects.SimpleGain
screenshot: /images/v2/reference/audio-modules/simplegain.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: Delay, impact: low, note: "Adds delay line processing when > 0ms" }
    - { parameter: Width, impact: low, note: "Adds mid/side encoding when != 100%" }
seeAlso:
  - { id: Delay, type: alternative, reason: "Full-featured delay with feedback and tempo sync, versus SimpleGain's static delay for timing alignment" }
  - { id: "core.gain", type: scriptnode, reason: "Scriptnode gain node with smoothed dB control" }
commonMistakes:
  - title: "SimpleGain delay has no feedback"
    wrong: "Using SimpleGain's Delay parameter for echo effects"
    right: "SimpleGain's delay is a static offset with no feedback. Use the Delay module for echo repeats."
    explanation: "The delay in SimpleGain is designed for timing alignment (e.g. compensating for latency), not for creating audible echo effects. It has no feedback path."
  - title: "Width=0 produces mono, not silence"
    wrong: "Setting Width to 0 expecting silence"
    right: "Width=0 produces a mono signal (mid only), not silence. Width=100 is unchanged stereo."
    explanation: "The Width parameter works on the mid/side balance. At 0% only the mid (mono) signal remains. At 200% the side signal is exaggerated."
forumReferences:
  - id: 1
    title: "Use SimpleGain over sound generator gain for smooth automation"
    summary: "SimpleGain applies a 50ms smoothed gain ramp; sound generators' own Gain parameter has no smoothing and can cause clicks on automated changes."
    topic: 8822
  - id: 2
    title: "Width has no effect on mono sources"
    summary: "Mid/side width processing scales the side signal (L-R); if both channels are identical the side is zero and Width has no audible effect."
    topic: 7956
  - id: 3
    title: "SimpleGain does not attenuate a multi-channel Sampler below 0dB"
    summary: "When a Sampler uses multiple mic channels, a single SimpleGain provides boost but gain reduction has no audible effect due to signal doubling via routing — use one SimpleGain per mic channel."
    topic: 1252
  - id: 4
    title: "Pan modulation requires Balance knob offset from centre"
    summary: "Pan modulation scales the existing Balance offset, so with Balance at centre (0) the modulation has zero range and no audible effect."
    topic: 6062
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: simple
  description: "Gain, delay, and mid/side width nodes in a scriptnode network"
llmRef: |
  Simple Gain (MasterEffect)

  Utility gain processor with optional delay, stereo width (mid/side), balance (pan), and polarity inversion. Four independent modulation chains control Gain, Delay, Width, and Balance. Processing order: polarity inversion -> delay -> gain -> width -> balance.

  Signal flow:
    audio in -> invert polarity (if on) -> delay (if > 0ms) -> smoothed gain -> mid/side width (if != 100%) -> balance pan -> audio out

  CPU: low, monophonic (MasterEffect). Delay and width add moderate cost when active.

  Parameters:
    Gain (-100 to 36 dB, default 0 dB) - output gain with 50ms smoothing
    Delay (0-500 ms, default 0 ms) - static delay with crossfade on time change
    Width (0-200%, default 100%) - stereo width via mid/side. 0=mono, 100=unchanged, 200=exaggerated sides.
    Balance (pan, default centre) - stereo pan position with 1000ms smoothing
    InvertPolarity (Off/On, default Off) - flips signal phase

  Modulation chains:
    Gain Modulation - scales gain per-block
    Delay Modulation - scales delay time per-block
    Width Modulation - interpolates between 1.0 and width value per-block
    Pan Modulation - scales balance value per-block

  When to use:
    Level automation, timing alignment, stereo width adjustment, panning, phase correction. The most common utility effect in HISE signal chains.

  Common mistakes:
    Delay is static with no feedback - use the Delay module for echoes.
    Width=0 is mono, not silence.

  Custom equivalent:
    scriptnode HardcodedFX: gain, delay, and mid/side nodes.

  See also:
    alternative Delay - full-featured delay with feedback and tempo sync
    [scriptnode] core.gain -- scriptnode gain node with smoothed dB control
---

::category-tags
---
tags:
  - { name: mixing, desc: "Effects that control volume, stereo width, or stereo balance" }
  - { name: utility, desc: "Modules for analysis, placeholders, or structural purposes without audio processing" }
---
::

![Simple Gain screenshot](/images/v2/reference/audio-modules/simplegain.png)

A utility effect that combines gain control, static delay, stereo width adjustment, stereo balance (panning), and polarity inversion into a single module. Despite its name, it offers more than simple gain - it is the standard utility module for level automation, timing alignment, and stereo image control.

Each of the four main parameters (Gain, Delay, Width, Balance) has its own modulation chain. The processing order is fixed: polarity inversion is applied first, followed by delay, gain, width, and finally balance. Features that are at their default values (delay=0, width=100%, balance=centre) are skipped entirely, keeping CPU usage minimal.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gain:
      desc: "Output gain in decibels with 50ms smoothing"
      range: "-100 - 36 dB"
      default: "0 dB"
    Delay:
      desc: "Static delay time with crossfade on time change"
      range: "0 - 500 ms"
      default: "0 ms"
    Width:
      desc: "Stereo width via mid/side processing"
      range: "0 - 200%"
      default: "100%"
    Balance:
      desc: "Stereo pan position"
      range: "L - C - R"
      default: "C"
    InvertPolarity:
      desc: "Flips signal phase (multiply by -1)"
      range: "Off / On"
      default: "Off"
  functions:
    midSideWidth:
      desc: "Encodes L/R to mid/side, scales sides by the width factor, then decodes back to L/R"
  modulations:
    GainModulation:
      desc: "Scales the gain parameter per block"
      scope: "monophonic"
    DelayModulation:
      desc: "Scales the delay time per block"
      scope: "monophonic"
    WidthModulation:
      desc: "Interpolates between 1.0 (no change) and the set width value"
      scope: "monophonic"
    PanModulation:
      desc: "Scales the balance value per block"
      scope: "monophonic"
---

```
// Simple Gain - monophonic utility processor
// stereo in -> stereo out

process(left, right) {
    // 1. Polarity inversion (if enabled)
    if InvertPolarity:
        left  *= -1
        right *= -1

    // 2. Static delay (skipped if 0ms)
    if Delay > 0:
        delayTime = Delay * DelayModulation
        left  = delayLine(left, delayTime)
        right = delayLine(right, delayTime)

    // 3. Smoothed gain (50ms ramp)
    gainValue = dBToLinear(Gain) * GainModulation
    left  *= smoothed(gainValue)
    right *= smoothed(gainValue)

    // 4. Stereo width (skipped if 100%)
    if Width != 100:
        width = (Width/100 - 1) * WidthModulation + 1
        [left, right] = midSideWidth(left, right, width)

    // 5. Balance / pan (skipped if centred)
    if Balance != centre:
        pan = Balance * PanModulation
        left  *= panGainL(pan)
        right *= panGainR(pan)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Level
    params:
      - { name: Gain, desc: "Output gain in decibels. 0 dB is unity. Negative values attenuate, positive values boost (up to +36 dB). Changes are smoothed with a 50ms ramp to prevent clicks.", range: "-100 - 36 dB", default: "0 dB" }
      - { name: InvertPolarity, desc: "Multiplies both channels by -1, flipping the signal phase. Applied before all other processing. Useful for phase correction or mid/side techniques.", range: "Off / On", default: "Off" }
  - label: Delay
    params:
      - { name: Delay, desc: "Static delay applied to both channels. Designed for timing alignment, not echo effects (no feedback). When set to 0ms, the delay line is bypassed entirely. Changes crossfade over one buffer to prevent clicks.", range: "0 - 500 ms", default: "0 ms" }
  - label: Stereo
    params:
      - { name: Width, desc: "Stereo width using mid/side processing. At 0% the output is mono (mid only). At 100% the stereo image is unchanged. Above 100% the side signal is exaggerated, widening the stereo image. Maximum is 200%.", range: "0 - 200%", default: "100%" }
      - { name: Balance, desc: "Stereo pan position. Centre is the default. Smoothed with a 1000ms ramp for gradual panning transitions.", range: "L - C - R", default: "C" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Gain Modulation", desc: "Scales the output gain. A single modulation value is sampled per block and multiplied with the gain parameter.", scope: "monophonic", constrainer: "Any" }
  - { name: "Delay Modulation", desc: "Scales the delay time. The modulation value multiplies the Delay parameter to determine the actual delay.", scope: "monophonic", constrainer: "Any" }
  - { name: "Width Modulation", desc: "Interpolates between 1.0 (no width change) and the set Width value. At modulation=1.0, the full Width setting is applied. At modulation=0.0, width is 100% (unchanged).", scope: "monophonic", constrainer: "Any" }
  - { name: "Pan Modulation", desc: "Scales the Balance parameter. Uses Pan mode for bipolar modulation of the stereo position. The modulation multiplies the current Balance value, so with Balance at centre (0), Pan Modulation has no range and no audible effect — set Balance to a non-centre value first.", scope: "monophonic", constrainer: "Any" }
---
::

### Smoothing

The gain smoothing ramp is 50 ms, while the balance smoothing is 1000 ms. Sound generators' own Gain parameter has no smoothing, so use SimpleGain for click-free automated gain changes on samplers and other sound generators. [1]($FORUM_REF.8822$)

### Width Behaviour

When Width is exactly 100% (the default), mid/side processing is skipped entirely. Width has no effect on a mono source (where L equals R) because mid/side encoding produces zero side signal. To widen a mono source, create stereo differences first using a short delay (Haas effect) or chorus, then apply Width. [2]($FORUM_REF.7956$) Width above 100% amplifies the side signal only, which can introduce out-of-phase content that cancels when summed to mono.

### Multi-Channel Routing

When a Sampler uses multiple mic channels, placing a single SimpleGain in the Sampler's FX chain provides boost but gain reduction below 0 dB has no audible effect due to signal doubling from the multi-channel routing. Use one SimpleGain per mic channel, or place the Sampler inside a Container and attach SimpleGain there. [3]($FORUM_REF.1252$)

### Pan Modulation Range

Pan Modulation scales the existing Balance offset, so with Balance at centre (0), the modulation has zero range and no audible effect. Set Balance to a non-centre value (e.g. full right) to give the Pan Modulation chain a range to work within. [4]($FORUM_REF.6062$)

**See also:** $MODULES.Delay$ -- Full-featured delay with feedback and tempo sync, versus SimpleGain's static delay for timing alignment, $SN.core.gain$ -- scriptnode gain node with smoothed dB control
