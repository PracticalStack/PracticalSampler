---
title: Effects
description: All HISE effect modules - signal path, voice scopes, and module list
---

Effects process audio after the sound generator stage. Every sound generator has an FX chain that holds one or more effect modules, processed in series from top to bottom.

## Signal Path

Effects sit at the end of a sound generator's processing chain. Each effect receives the output of the previous one in series.

::signal-path
---
glossary:
  functions:
    renderVoices:
      desc: "Sound generation and modulation stages produce per-voice audio."
    processPolyFX:
      desc: "Polyphonic effects run independently per voice before summing. Each voice has its own effect state."
    sumVoices:
      desc: "Mixes all active voice outputs into a single stereo buffer."
    processMonoFX:
      desc: "Monophonic effects run on the summed output with access to pitch information from the last played note."
    processMasterFX:
      desc: "Master effects run on the final stereo signal. Most built-in effects operate at this scope."
---

```
// Effect processing within a sound generator

for each active voice:
    audio = renderVoices(voice)
    audio = processPolyFX(voice, audio)

output = sumVoices()
output = processMonoFX(output, lastPlayedPitch)
output = processMasterFX(output)
```

::

## Voice Scopes

The voice scope determines where in the signal path an effect operates and how many instances of it run simultaneously.

**Master Effects** process the summed stereo output of all voices as a single signal. This is the most common scope and the cheapest per instance since only one copy of the effect runs regardless of voice count. Use master effects for reverb, delay, EQ, dynamics, and most other standard processing.

**Monophonic Effects** also process after voice summing, but receive pitch information from the last played note. This enables frequency-tracking behaviour like harmonic filtering that follows the played pitch. Currently only the Harmonic Filter Monophonic uses this scope.

**Polyphonic Effects** process each voice independently before summing. Each active voice gets its own effect state, so a 16-voice patch runs 16 independent instances of the effect. This allows per-voice filtering, distortion, and stereo processing that responds to individual note modulation. The trade-off is higher CPU cost that scales with voice count.

## Common Parameters

Effects do not share a universal parameter set. Each effect defines its own parameters. The only common control is the bypass toggle inherited from the processor base class.

::parameter-table
---
groups:
  - label: State
    params:
      - { name: Bypass, desc: "Bypasses the effect. Master effects use a soft bypass with a short crossfade to avoid clicks.", range: "On / Off", default: "Off" }
---
::

## Chains

Unlike sound generators, effects do not have a standard set of modulation chains. Each effect module defines its own chains based on what it needs - for example, a send effect may expose a send level modulation chain, while a filter exposes frequency and resonance chains. See the individual module pages for available chains.

## Master Effects

Process the summed output of all voices as a single stereo signal. Most built-in effects are master effects.

- [Analyser]($MODULES.Analyser$): Provides audio visualization tools including goniometer, oscilloscope, and spectrum analyzer.
- [Chorus]($MODULES.Chorus$): Stereo chorus effect with modulated delay lines for thickening and movement.
- [Convolution Reverb]($MODULES.Convolution$): Zero-latency convolution reverb with adjustable dry/wet levels, predelay, damping, and high-cut filtering for shaping impulse responses.
- [Parametriq EQ]($MODULES.CurveEq$): A parametric equalizer with unlimited filter bands and an FFT spectrum display for visual feedback.
- [Delay]($MODULES.Delay$): Stereo delay with independent left and right times, feedback, and filtering, with optional tempo sync for rhythmic echo effects.
- [Dynamics]($MODULES.Dynamics$): A dynamics processor combining gate, compressor, and limiter based on chunkware's SimpleCompressor algorithms.
- [Empty]($MODULES.EmptyFX$): A placeholder effect that passes audio through unchanged, useful for routing or as a template.
- [Hardcoded Master FX]($MODULES.HardcodedMasterFX$): Runs a compiled C++ DSP network as a master effect, with dynamic parameter and complex data exposure from the network.
- [MidiMetronome]($MODULES.MidiMetronome$): A metronome that produces click sounds synchronized to a connected MIDI player's tempo.
- [Phase FX]($MODULES.PhaseFX$): Phaser effect using modulated allpass filters for sweeping frequency notches.
- [Routing Matrix]($MODULES.RouteFX$): Routing matrix for duplicating and distributing audio across channels, useful for building aux-style signal paths and complex channel layouts.
- [Saturator]($MODULES.Saturator$): Applies waveshaping saturation with pre/post gain controls and wet/dry mix.
- [Script FX]($MODULES.ScriptFX$): Processes audio through a scriptnode DSP network as a master effect, with scriptable parameters and complex data routing.
- [Send Effect]($MODULES.SendFX$): Routes audio to a send container with adjustable gain, channel offset, and optional smoothing for consistent send automation.
- [Shape FX]($MODULES.ShapeFX$): Waveshaper effect with selectable shaping modes, bias, filters, and oversampling, suitable for distortion and tone shaping with optional autogain.
- [Simple Gain]($MODULES.SimpleGain$): Utility gain processor with optional delay, stereo width, and balance control, useful for level automation, simple timing offsets, and mid-side shaping.
- [Simple Reverb]($MODULES.SimpleReverb$): Algorithmic reverb based on Freeverb with controls for room size, damping, and stereo width.
- [Effect Slot]($MODULES.SlotFX$): A placeholder for another effect that can be swapped dynamically.

## Monophonic Effects

Process audio after voice summing but with access to per-voice pitch information for frequency-tracking behaviour.

- [Harmonic Filter Monophonic]($MODULES.HarmonicFilterMono$): Monophonic peak filters tuned to the root frequency and harmonics of the last played note, with crossfadeable A/B configurations.

## Polyphonic Effects

Process each voice independently before summing, allowing per-voice effect state.

- [Hardcoded Polyphonic FX]($MODULES.HardcodedPolyphonicFX$): Runs a compiled C++ DSP network as a polyphonic effect, processing each voice independently with per-voice state.
- [Harmonic Filter]($MODULES.HarmonicFilter$): Polyphonic peak filters tuned to the root frequency and harmonics of each voice, with crossfadeable A/B slider pack configurations.
- [Noise Grain Player]($MODULES.NoiseGrainPlayer$): A polyphonic granular noise player that blends an audio file with white noise at a configurable grain size.
- [Polyphonic Script FX]($MODULES.PolyScriptFX$): Processes each voice independently through a scriptnode DSP network, with per-voice state and polyphonic modulation support.
- [Filter]($MODULES.PolyphonicFilter$): Applies monophonic or polyphonic filtering with modulatable frequency, gain, and resonance, supporting multiple filter types.
- [Polyshape FX]($MODULES.PolyshapeFX$): A polyphonic waveshaper with multiple shaping modes, table-based curves, and optional oversampling.
- [Stereo FX]($MODULES.StereoFX$): Polyphonic stereo panner with width control and modulatable pan position.
