---
title: Sine Wave Generator
moduleId: SineSynth
type: SoundGenerator
subtype: SoundGenerator
tags: [oscillator]
builderPath: b.SoundGenerators.SineSynth
screenshot: /images/v2/reference/audio-modules/sinesynth.png
cpuProfile:
  baseline: very low
  polyphonic: true
  scalingFactors: [voice count, saturation]
seeAlso:
  - { id: "core.oscillator", type: scriptnode, reason: "Scriptnode multi-waveform oscillator (includes sine mode)" }
commonMistakes:
  - title: "Use scriptnode for additive synthesis"
    wrong: "Using multiple SineSynth modules for additive synthesis"
    right: "Write a custom scriptnode network for additive synthesis"
    explanation: "Each SineSynth instance carries its own pitch modulation and event handling overhead. For additive synthesis with many partials, a single scriptnode network with multiple oscillator nodes is far more efficient."
  - title: "SaturationAmount clamped to 99% maximum"
    wrong: "Setting SaturationAmount to exactly 1.0 and expecting maximum distortion"
    right: "Use values up to 0.99 for maximum saturation"
    explanation: "A value of 1.0 is internally clamped to 0.99 because the waveshaping formula would produce silence at exactly 1.0."
  - title: "Tuning modes are mutually exclusive"
    wrong: "Adjusting OctaveTranspose or SemiTones while UseFreqRatio is enabled"
    right: "Disable UseFreqRatio first, then adjust OctaveTranspose and SemiTones"
    explanation: "The two tuning modes are mutually exclusive. When UseFreqRatio is On, only CoarseFreqRatio and FineFreqRatio affect the pitch."
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: low
  description: "A single sine oscillator node with optional waveshaper and pitch ratio control"
llmRef: |
  Sine Wave Generator (SoundGenerator)

  Lightweight polyphonic sine wave oscillator with two tuning modes and optional saturation waveshaping. Designed for sub-bass layers, FM carrier/modulator tones, and adding subtle harmonics.

  Signal flow:
    MIDI note -> pitch calculation (tuning factor) -> phase accumulator -> sine table lookup -> saturation -> gain modulation -> mono-to-stereo -> effect chain -> audio out

  CPU: very low per voice, polyphonic

  Parameters:
    OctaveTranspose (-5 to 5 octaves, default 0) - coarse pitch offset in musical mode
    SemiTones (-12 to 12 semitones, default 0) - fine pitch offset in musical mode
    UseFreqRatio (Off/On, default Off) - switches to harmonic ratio tuning mode
    CoarseFreqRatio (-5 to 16, default 1) - harmonic multiplier in ratio mode
    FineFreqRatio (0-1, default 0) - fractional frequency offset in ratio mode
    SaturationAmount (0-100%, default 0%) - soft-clipping waveshaping to add harmonics
    Gain (0-100%, default 25%) - output volume as linear gain, modulatable via Gain Modulation chain
    Balance (-1 to 1, default 0) - stereo balance
    VoiceLimit (1-256, default 256) - maximum polyphony
    KillFadeTime (0-20000 ms, default 20 ms) - fade-out when voices are killed

  Modulation chains:
    Gain Modulation - scales the output volume (linked to Gain parameter)
    Pitch Modulation - scales the pitch of all voices (standalone, per-sample)

  When to use:
    Sub-bass oscillator, FM synthesis carrier or modulator, test tone, layering a sine undertone beneath samples. Not recommended for additive synthesis with many partials due to per-instance overhead.

  Common mistakes:
    Multiple SineSynth instances for additive synthesis - use scriptnode instead.
    SaturationAmount = 1.0 produces silence (clamped to 0.99 internally).
    OctaveTranspose/SemiTones ignored when UseFreqRatio is On (mutually exclusive).

  Custom equivalent:
    scriptnode SoundGenerator with sine oscillator node.

  See also:
    [scriptnode] core.oscillator -- scriptnode multi-waveform oscillator (includes sine mode)
---

::category-tags
---
tags:
  - { name: oscillator, desc: "Modules that generate audio or modulation signals from oscillators or synthesis algorithms" }
---
::

![Sine Wave Generator screenshot](/images/v2/reference/audio-modules/sinesynth.png)

The Sine Wave Generator produces a pure sine tone from a 2048-sample lookup table with linear interpolation. It is the lightest sound generator in HISE, designed for sub-bass layers, FM synthesis building blocks, and adding subtle harmonics beneath sampled or synthesised sounds.

Two tuning modes are available. Musical mode offsets the pitch by octaves and semitones. Ratio mode sets the frequency as a harmonic multiplier of the root note, useful for FM synthesis where carrier-to-modulator ratios define the timbre. A saturation waveshaper can add harmonics to the pure sine, progressively driving it towards a soft-clipped shape.

## Signal Path

::signal-path
---
glossary:
  parameters:
    OctaveTranspose:
      desc: "Coarse pitch offset in octaves (musical tuning mode)"
      range: "-5 - 5"
      default: "0"
    SemiTones:
      desc: "Fine pitch offset in semitones (musical tuning mode)"
      range: "-12 - 12"
      default: "0"
    UseFreqRatio:
      desc: "Switches between musical and harmonic ratio tuning"
      range: "Off / On"
      default: "Off"
    CoarseFreqRatio:
      desc: "Harmonic frequency multiplier (ratio tuning mode)"
      range: "-5 - 16"
      default: "1"
    FineFreqRatio:
      desc: "Fractional frequency offset (ratio tuning mode)"
      range: "0 - 1"
      default: "0"
    SaturationAmount:
      desc: "Soft-clipping waveshaping amount"
      range: "0 - 100%"
      default: "0%"
    Gain:
      desc: "Output volume as normalised linear gain"
      range: "0 - 100%"
      default: "25%"
    Balance:
      desc: "Stereo balance applied after mono-to-stereo conversion"
      range: "-1 - 1"
      default: "0"
  functions:
    sineLookup:
      desc: "Reads the sine value from a 2048-sample table with linear interpolation"
    saturate:
      desc: "Soft-clipping waveshaper: (1+k)*x / (1+k*|x|), where k is derived from SaturationAmount"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Multiplies into the phase increment per sample"
      scope: "per-voice"
---

```
// Sine Wave Generator - per-voice processing
// polyphonic, one voice per note

// Pitch calculation (on note start)
freq = midiToHz(noteNumber)
if UseFreqRatio:
    factor = ratioFromCoarseAndFine(CoarseFreqRatio, FineFreqRatio)
else:
    factor = 2^(OctaveTranspose + SemiTones / 12)

phaseIncrement = freq * factor / sampleRate

// Per-sample generation
phase += phaseIncrement * PitchModulation
value = sineLookup(phase)

// Saturation (if enabled)
if SaturationAmount > 0:
    value = saturate(value, SaturationAmount)

// Output
output = value * Gain * GainModulation
left = output
right = output    // mono copied to stereo
```

::

## Parameters

::parameter-table
---
groups:
  - label: Tuning (Musical)
    params:
      - { name: OctaveTranspose, desc: "Coarse pitch offset in whole octaves. Only active when UseFreqRatio is Off.", range: "-5 - 5", default: "0" }
      - { name: SemiTones, desc: "Fine pitch offset in semitones. Combined with OctaveTranspose. Only active when UseFreqRatio is Off.", range: "-12 - 12", default: "0" }
  - label: Tuning (Ratio)
    params:
      - { name: UseFreqRatio, desc: "Switches between musical tuning (octave/semitone) and harmonic ratio tuning. The two modes are mutually exclusive.", range: "Off / On", default: "Off" }
      - { name: CoarseFreqRatio, desc: "Harmonic frequency multiplier. A value of 1 is the root frequency, 2 is one octave up (second harmonic), 3 is the third harmonic, and so on. Negative values use exponential (octave-based) division. Only active when UseFreqRatio is On.", range: "-5 - 16", default: "1" }
      - { name: FineFreqRatio, desc: "Fractional offset added to the frequency ratio for fine-tuning. Added linearly to the coarse ratio. Only active when UseFreqRatio is On.", range: "0 - 1", default: "0" }
  - label: Waveshaping
    params:
      - { name: SaturationAmount, desc: "Amount of soft-clipping saturation applied to the sine wave. At 0% the output is a pure sine. Higher values progressively add odd harmonics, approaching a square-like shape. Internally clamped to 99% maximum.", range: "0 - 100%", default: "0%" }
  - label: Output
    params:
      - { name: Gain, desc: "Output volume as normalised linear gain (not decibels). Modulatable via the Gain Modulation chain.", range: "0 - 100%", default: "25%" }
      - { name: Balance, desc: "Stereo balance. Applied by the base class after per-voice processing.", range: "-1 - 1", default: "0" }
  - label: Voice Management
    params:
      - { name: VoiceLimit, desc: "Maximum number of simultaneous voices.", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Fade-out time when voices are killed by exceeding the voice limit or by a voice killer.", range: "0 - 20000 ms", default: "20 ms" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Gain Modulation", desc: "Scales the output volume. Applied as a per-voice multiply after sine generation and saturation.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Modulates the pitch of all voices. Applied per-sample as a multiplier on the phase increment.", scope: "per-voice", constrainer: "Any" }
---
::

### Tuning Modes

The two tuning modes are mutually exclusive. When UseFreqRatio is Off, OctaveTranspose and SemiTones set the pitch offset using the formula `2^(octaves + semitones/12)`. When UseFreqRatio is On, CoarseFreqRatio and FineFreqRatio define a harmonic multiplier. Changing the inactive mode's parameters has no effect until that mode is selected.

### Ratio Mode Harmonics

In ratio mode, the coarse ratio maps directly to harmonic numbers: 1 = fundamental, 2 = second harmonic (octave), 3 = third harmonic (octave + fifth), and so on. For sub-harmonics, negative coarse values use exponential division. The fine ratio is always added as a linear offset for detuning.

### Saturation Character

The saturation waveshaper uses a soft-clipping transfer function that progressively adds odd harmonics to the sine wave. At low values it produces a subtle warmth; at high values the waveform approaches a clipped shape with audible overtones. This is useful for thickening a sine bass without needing a separate effect.

### Output Routing

The sine output is generated in mono and copied to both channels. The Balance parameter is applied by the base class after the voice buffer is complete. An effect chain slot is available for per-voice effects.

**See also:** $SN.core.oscillator$ -- scriptnode multi-waveform oscillator (includes sine mode)
