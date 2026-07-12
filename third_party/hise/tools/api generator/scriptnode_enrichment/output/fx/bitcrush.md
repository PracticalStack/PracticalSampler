---
title: Bitcrush
description: "Reduces the bit depth of the audio signal, producing quantisation noise and digital distortion."
factoryPath: fx.bitcrush
factory: fx
polyphonic: true
tags: [fx, lo-fi, distortion]
screenshot: /images/v2/reference/scriptnodes/fx/bitcrush.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "fx.sampleandhold", type: companion, reason: "Sample rate reduction counterpart - bitcrush reduces amplitude resolution, sampleandhold reduces time resolution" }
commonMistakes:
  - title: "DC Offset mode introduces DC bias"
    wrong: "Using DC Offset mode without a DC blocker downstream"
    right: "Use Bipolar mode for most applications, or add a high-pass filter after DC Offset mode to remove the bias."
    explanation: "DC Offset mode rounds using ceil with a half-step shift, which introduces a small DC offset into the signal. Bipolar mode rounds toward zero and does not produce DC bias."
  - title: "BitDepth 16 is effectively transparent"
    wrong: "Setting BitDepth to 16 expecting audible distortion"
    right: "Use lower BitDepth values (4-10) for audible bitcrushing. At 16 bits the quantisation noise is below the audible threshold."
    explanation: "At 16 bits there are 65536 quantisation levels, producing noise well below audible threshold. The effect becomes clearly audible below roughly 10-12 bits."
  - title: "Clipping does not remove DC offset"
    wrong: "Placing a clip node after DC Offset mode to remove the DC bias"
    right: "Use a high-pass filter (even a single-pole HPF at ~20 Hz) after the node to remove DC offset."
    explanation: "DC offset is a 0 Hz signal component, not an amplitude excess. Clipping to [-1, 1] has no effect on it. A high-pass filter is the correct remedy."
llmRef: |
  fx.bitcrush

  Reduces the bit depth of the audio signal by quantising each sample to a stepped curve. Produces the characteristic lo-fi, gritty sound of low bit-depth digital audio. Each voice maintains its own bit depth value; Mode is shared across all voices.

  Signal flow:
    audio in -> quantise to 2^BitDepth levels (rounding depends on Mode) -> audio out

  CPU: low, polyphonic

  Parameters:
    BitDepth (4.0 - 16.0, step 0.1, default 16.0) - number of quantisation bits. Fractional values supported. 16 = transparent, 4 = heavy distortion. Not smoothed - rapid audio-rate modulation can alias; use dynamic_blocksize for frame processing.
    Mode (DC / Bipolar, default DC) - rounding behaviour. DC Offset rounds up (introduces DC bias). Bipolar rounds toward zero (symmetric, no DC bias). Use Bipolar when modulating BitDepth to avoid time-varying DC drift.

  Vintage DAC emulation:
    Wrap with mu-law compression before and expansion after to distribute quantisation logarithmically (more resolution for quiet signals). Models vintage drum machines and 8-bit samplers more accurately than linear bit reduction.

  When to use:
    Lo-fi effects, digital distortion, retro character. Pair with fx.sampleandhold for combined bit-depth and sample-rate reduction.

  Common mistakes:
    DC Offset mode introduces DC bias - use Bipolar for most cases.
    BitDepth 16 is transparent - use lower values for audible effect.
    Clipping does not remove DC offset -- use a high-pass filter instead.

  See also:
    companion fx.sampleandhold - sample rate reduction counterpart
---

Reduces the bit depth of the audio signal by quantising each sample to a stepped curve. Lower bit depths produce fewer quantisation levels and more pronounced digital distortion, creating the characteristic gritty sound of low bit-depth audio. Fractional bit depth values are supported, allowing smooth modulation of the distortion intensity.

Two modes control how the quantisation rounds values. **DC Offset** mode rounds upward and shifts by half a step, which introduces a small DC bias but produces one extra quantisation step. **Bipolar** mode rounds toward zero for both positive and negative samples, producing symmetric distortion with no DC offset. Bipolar mode is generally preferred unless the DC offset behaviour is specifically desired.

Each voice maintains its own bit depth value, so BitDepth can be modulated independently per voice. The Mode setting is shared across all voices - changing it affects every active voice simultaneously.

## Signal Path

::signal-path
---
glossary:
  parameters:
    BitDepth:
      desc: "Number of quantisation bits (fractional values supported)"
      range: "4.0 - 16.0"
      default: "16.0"
    Mode:
      desc: "Rounding behaviour: DC Offset (ceil + shift) or Bipolar (round toward zero)"
      range: "DC / Bipolar"
      default: "DC"
  functions:
    quantise:
      desc: "Snaps each sample to the nearest quantisation level based on bit depth and mode"
---

```
// fx.bitcrush - bit depth reduction
// audio in -> audio out

process(input) {
    stepSize = 1.0 / pow(2, BitDepth)

    if Mode == DC:
        output = stepSize * ceil(input / stepSize) - 0.5 * stepSize
    else:  // Bipolar
        output = quantise(input, stepSize)  // round toward zero
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: BitDepth, desc: "Number of quantisation bits. Lower values produce heavier distortion. Fractional values are supported for smooth modulation between quantisation levels. At 16.0 the quantisation noise is below the audible threshold.", range: "4.0 - 16.0", default: "16.0", hints: [{ type: warning, text: "Not smoothed internally. Rapid modulation at audio rates can produce aliasing artefacts. Insert a `dynamic_blocksize` node to switch to frame processing if high-rate modulation is needed." }] }
  - label: Configuration
    params:
      - { name: Mode, desc: "Rounding behaviour for the quantisation. DC Offset mode rounds upward with a half-step shift (introduces DC bias). Bipolar mode rounds toward zero (symmetric, no DC bias).", range: "DC / Bipolar", default: "DC", hints: [{ type: tip, text: "Use **Bipolar** when modulating BitDepth. DC mode injects a time-varying DC component as the bit depth changes, which can cause audible low-frequency drift." }] }
---
::

### Vintage DAC Emulation

For a more authentic vintage DAC character (drum machines, 8-bit samplers), place a mu-law compression node before fx.bitcrush and a mu-law expansion node after it. This distributes quantisation steps logarithmically - more resolution for quiet signals, less for loud ones - which improves the signal-to-noise ratio at low bit depths compared to linear bit reduction alone.

**See also:** $SN.fx.sampleandhold$ -- sample rate reduction counterpart, often paired for combined lo-fi effects
