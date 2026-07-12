---
title: Sound Generators
description: All HISE sound generator modules - signal path, routing, parameters, and module list
---

Sound generators produce audio output and form the backbone of the signal chain. They host modulator chains for gain and pitch, and can contain an effect chain.

## Signal Path

![Sound Generator Signal Path](/images/v2/reference/audio-modules/soundgenerators2.svg)

::signal-path
---
glossary:
  functions:
    processMidiChain:
      desc: "Passes each MIDI event through the MIDI processor chain. Processors can filter, transform, or generate events. Ignored events are skipped."
    renderVoice:
      desc: "Generates audio output for a single voice. The implementation depends on the sound generator type (oscillator, sampler, scriptnode network, etc.)."
    sumVoices:
      desc: "Mixes all active voice outputs into a single stereo buffer."
    addToOutput:
      desc: "Adds this sound generator's output to the parent signal. Multiple sound generators in a container are summed together."
  modulations:
    monoPitchMod:
      desc: "Monophonic pitch modulation signal, calculated once and shared across all voices."
      scope: "monophonic"
    polyPitchMod:
      desc: "Per-voice pitch modulation applied before voice rendering."
      scope: "per-voice"
    polyGainMod:
      desc: "Per-voice gain modulation applied after voice rendering."
      scope: "per-voice"
    monoGainMod:
      desc: "Monophonic gain modulation applied to the summed voice output."
      scope: "monophonic"
  parameters:
    polyFX:
      desc: "Polyphonic effects processed independently per voice before summing."
    monoFX:
      desc: "Monophonic (master) effects processed on the final summed output."
---

```
// Sound Generator - render one audio block

processMidiChain(midiBuffer)

monoPitchMod = calculateMonoPitchModulation()

for each active voice:
    pitch = monoPitchMod * polyPitchMod
    audio = renderVoice(voice, pitch)
    audio = audio * polyGainMod
    audio = polyFX(audio)

output = sumVoices()
output = output * monoGainMod
output = monoFX(output)

addToOutput(output)
```

::

## Multichannel Routing

Sound generators can process multiple channels for use cases like multimic samples, AUX sends, and parallel FX processing.

![Routing Matrix](/images/v2/reference/audio-modules/routing_matrix.png)

Access the routing matrix by clicking the volume display icon next to the sound generator's name. Right-click the matrix to change the channel count, or set it via script:

```javascript
const var MasterChain = Synth.getRoutingMatrix("Master Chain");
MasterChain.setNumChannels(8);
```

### Scriptnode and Hardcoded Module Channels

Scriptnode and hardcoded modules determine their channel count from the network's top-level container. The default is stereo (2 channels), but multichannel configurations of up to 16 channels are supported.

The hosting module passes its channel count to the network during initialisation. If there is a mismatch between the module's channel count and the network's expected channel count, extra channels are zero-padded and excess channels are truncated. For hardcoded (compiled) modules, the channel count is fixed at compile time — it cannot be changed without recompilation. The routing matrix channel count must match the compiled network's channel count.

Use the routing matrix to map network output channels to the desired bus layout, or to configure multi-channel input for effect modules.

## Common Parameters

::parameter-table
---
groups:
  - label: Output
    params:
      - { name: Gain, desc: "Output volume as normalised linear gain (not decibels). Modulatable via the Gain Modulation chain.", range: "0 - 100%", default: "100%" }
      - { name: Balance, desc: "Stereo balance. Applied after per-voice processing.", range: "-1 - 1", default: "0" }
  - label: Voice Management
    params:
      - { name: VoiceLimit, desc: "Maximum number of simultaneous voices.", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Fade-out time when voices are killed by exceeding the voice limit or by a voice killer.", range: "0 - 20000 ms", default: "20 ms" }
---
::

## Chains

::modulation-table
---
chains:
  - { name: "MIDI", desc: "Every MIDI message received by the sound generator is processed by this chain. Ignored messages are not passed to child modules.", scope: "per-event", constrainer: "Any" }
  - { name: "Gain Modulation", desc: "Scales the output volume per voice.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Modulates the pitch of all voices. Applied per-sample as a multiplier on the phase increment.", scope: "per-voice", constrainer: "Any" }
  - { name: "FX", desc: "The effect chain of this module.", scope: "post-voice", constrainer: "Any" }
---
::

## Modules

- [Audio Loop Player]($MODULES.AudioLooper$): A single-file audio player with looping, pitch tracking, tempo sync, and reverse playback.
- [Global Modulator Container]($MODULES.GlobalModulatorContainer$): A container that processes Modulator instances that can be used at different locations.
- [Hardcoded Synthesiser]($MODULES.HardcodedSynth$): Runs a compiled C++ DSP network as a polyphonic sound generator with per-voice processing and full modulator chain support.
- [Macro Modulation Source]($MODULES.MacroModulationSource$): A container that hosts modulator chains whose output drives the macro control system.
- [Noise Generator]($MODULES.Noise$): A white noise generator useful for layering, testing signal flow, or as a modulation source.
- [Scriptnode Synthesiser]($MODULES.ScriptSynth$): Generates polyphonic audio from a scriptnode DSP network, with per-voice processing and full modulator chain support.
- [Send Container]($MODULES.SendContainer$): A signal chain tool that receives the signal from a Send FX and applies its own effect chain.
- [Silent Synth]($MODULES.SilentSynth$): A silent sound generator that routes signals through its effect chain without producing audio of its own.
- [Sine Wave Generator]($MODULES.SineSynth$): A lightweight sine wave generator for FM synthesis, additive synthesis, or adding subtle harmonics to other sounds.
- [Sampler]($MODULES.StreamingSampler$): A disk-streaming sampler with sample maps, round robin, crossfade groups, and timestretching.
- [Container]($MODULES.SynthChain$): A container for other Sound generators.
- [Synthesiser Group]($MODULES.SynthGroup$): A container for synthesisers that share common modulation, with optional FM synthesis and unison detune/spread.
- [Waveform Generator]($MODULES.WaveSynth$): A waveform generator based on BLIP synthesis of common synthesiser waveforms.
- [Wavetable Synthesiser]($MODULES.WavetableSynth$): A two-dimensional wavetable synthesiser that morphs between waveforms using a table index and supports audio file resynthesis.
