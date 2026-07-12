---
title: Noise Generator
moduleId: Noise
type: SoundGenerator
subtype: SoundGenerator
tags: [oscillator]
builderPath: b.SoundGenerators.Noise
screenshot: /images/v2/reference/audio-modules/noise.png
cpuProfile:
  baseline: very low
  polyphonic: true
  scalingFactors: [voice count]
seeAlso: []
commonMistakes:
  - title: "Noise does not render in FX plugins by default"
    wrong: "Adding a Noise Generator to an FX plugin project and expecting audio output"
    right: "Enable **Sound Generators FX** in Project Preferences before exporting"
    explanation: "Sound generators are not rendered in FX plugin builds unless this setting is explicitly enabled."
forumReferences:
  - id: 1
    title: "Sound generators silent in FX plugin exports"
    summary: "Sound generators are not rendered when exporting as an FX plugin unless 'Sound Generators FX' is enabled in Project Preferences."
    topic: 701
  - id: 2
    title: "Trigger continuous noise with Synth.playNote()"
    summary: "To start a noise generator continuously from a script (e.g. for vinyl crackle), call Synth.playNote() with the desired note and velocity from an onInit or UI callback."
    topic: 2206
  - id: 3
    title: "Audio-rate noise cannot be routed outside its scriptnode network"
    summary: "Audio-rate signals generated inside a scriptnode network cannot be routed to HISE modules outside that network; modulation of external parameters must stay at control rate."
    topic: 8522
llmRef: |
  Noise Generator (SoundGenerator)

  Polyphonic white noise generator. Produces full-spectrum random noise at audio rate. Designed for layering noise textures beneath sampled or synthesised sounds, testing signal flow, and as a broadband source for filtering.

  Signal flow:
    Note-on -> white noise generation (per sample) -> gain modulation -> mono-to-stereo -> effect chain -> audio out

  CPU: very low per voice, polyphonic

  Parameters:
    Gain (0-100%, default 25%) - output volume as linear gain, modulatable via Gain Modulation chain
    Balance (-1 to 1, default 0) - stereo balance
    VoiceLimit (1-256, default 256) - maximum polyphony
    KillFadeTime (0-20000 ms, default 20 ms) - fade-out when voices are killed

  Modulation chains:
    Gain Modulation - scales the output volume (linked to Gain parameter)
    Pitch Modulation - has no audible effect on noise (noise has no pitch)

  When to use:
    White noise layer for breath/air textures, broadband source for subtractive synthesis with a filter effect, test signal for verifying signal flow, ambient noise bed triggered via Synth.playNote().

  Limitations:
    Only white noise (no pink, brown, or other noise colours). Use scriptnode for shaped noise spectra.
    FX plugin builds require Sound Generators FX enabled in Project Preferences.

  Common mistakes:
    Adding to an FX plugin without enabling Sound Generators FX - produces silence.

  See also: none
---

::category-tags
---
tags:
  - { name: oscillator, desc: "Modules that generate audio or modulation signals from oscillators or synthesis algorithms" }
---
::

![Noise Generator screenshot](/images/v2/reference/audio-modules/noise.png)

The Noise Generator produces polyphonic white noise. Each voice generates independent random samples at audio rate, producing a full-spectrum broadband signal. It is the simplest sound generator in HISE, useful for layering breath or air textures beneath samples, as a broadband source for subtractive synthesis when followed by a filter, or as a test signal for verifying signal flow.

The output is mono white noise copied to both stereo channels. Shape the spectrum with effects in the FX chain - a bandpass filter creates wind or breath textures, a highpass creates presence/air, and a lowpass produces rumble.

### FX Plugin Builds

Sound generators are not rendered in exported FX plugins by default. To use a Noise Generator in an FX plugin, enable **Sound Generators FX** in Project Preferences before compiling. Without this setting, the module produces silence in the exported plugin even though it works in the HISE IDE. [1]($FORUM_REF.701$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gain:
      desc: "Output volume as normalised linear gain"
      range: "0 - 100%"
      default: "25%"
    Balance:
      desc: "Stereo balance applied after mono-to-stereo conversion"
      range: "-1 - 1"
      default: "0"
  functions:
    noiseGen:
      desc: "Generates a random sample value per audio sample"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Inherited pitch chain - has no audible effect on noise"
      scope: "per-voice"
---

```
// Noise Generator - per-voice processing
// polyphonic, one voice per note

// Per-sample generation
value = random(-1.0, 1.0)    // white noise

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
  - { name: "Gain Modulation", desc: "Scales the output volume. Applied as a per-voice multiply after noise generation.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Inherited pitch modulation chain. Has no audible effect on white noise since noise has no fundamental frequency, but is present for compatibility with the sound generator architecture.", scope: "per-voice", constrainer: "Any" }
---
::

### Continuous Noise via Scripting

To start a noise generator playing continuously (for ambient noise, vinyl crackle, or similar effects), call `Synth.playNote()` from a script callback with a long or infinite note duration. This avoids needing a held key to keep the noise running. [2]($FORUM_REF.2206$)

### Noise as a Modulation Source

Audio-rate noise generated inside a scriptnode network cannot be routed to HISE modules outside that network. [3]($FORUM_REF.8522$) For noise-based modulation of external parameters, use a control-rate approach such as a `sampleAndHold` node fed by a noise source, which produces a stepped random modulation signal at a controllable rate.
