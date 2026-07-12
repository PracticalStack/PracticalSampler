---
title: Audio Modules
description: Reference for all HISE audio modules — sound generators, modulators, effects, and MIDI processors

guidance:
  summary: >
    Complete reference for all HISE processor types. Sound Generators:
    StreamingSampler (disk-streaming sampler), WaveSynth (basic waveforms),
    WavetableSynth, SineSynth (FM/additive), AudioLooper, SynthGroup,
    ModulatorSynthChain. Modulators: AhdsrEnvelope, SimpleEnvelope, LFO,
    Velocity, MidiController, Random, KeyNumber, GlobalModulator.
    Effects: PolyphonicFilter, CurveEq, SimpleReverb, Convolution, Delay,
    Chorus, PhaseFX, Saturator, SimpleGain, SlotFX, Dynamics, Analyser,
    ShapeFX. MIDI Processors: ScriptProcessor, Transposer, MidiMuter.
    Each entry includes all parameters, usage notes, and per-voice vs
    master placement guidance.
  concepts:
    - audio modules reference
    - processor types
    - sound generators
    - modulators
    - effects
    - MIDI processors
  complexity: advanced
---

Audio modules are the building blocks of every HISE instrument. They form a tree structure rooted at a top-level Container (ModulatorSynthChain), with each module processing audio, MIDI, or control signals.

## Module Types

**[Sound Generators](/v2/reference/audio-modules/sound-generators/)** produce audio output and form the backbone of the signal chain. They host MIDI processor, modulation, and effect chains.

**[MIDI Processors](/v2/reference/audio-modules/midi-processors/)** transform, filter, or generate MIDI events before they reach the sound generator's voice rendering.

**[Modulators](/v2/reference/audio-modules/modulators/)** generate control signals in the range 0.0 to 1.0 that modulate gain, pitch, and effect parameters. Three subtypes cover different timing needs: voice start, time variant, and envelope.

**[Effects](/v2/reference/audio-modules/effects/)** process audio after generation. They operate at master, monophonic, or polyphonic scope depending on whether they need per-voice state.

## Reading the Pseudocode

This documentation uses interactive pseudocode blocks to describe signal flow and processing models. Highlighted terms are hoverable - they show descriptions, parameter ranges, and implementation details in a tooltip.

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "A module parameter. Blue terms are configurable values exposed via setAttribute()."
      range: "20 - 20000 Hz"
      default: "1000 Hz"
  functions:
    processAudio:
      desc: "A processing function. Orange terms describe computation steps performed by the module."
    applyFilter:
      desc: "Functions can include a detail field with formulas or mode-specific behaviour."
      detail: |
        LowPass:  attenuate frequencies above cutoff
        HighPass: attenuate frequencies below cutoff
        BandPass: attenuate frequencies outside band
  modulations:
    GainMod:
      desc: "A modulation signal. Green terms represent control signals from modulators."
      scope: "per-voice"
---

```
// Example pseudocode - hover the highlighted terms
output = processAudio(input, Frequency)
output = applyFilter(output)
output = output * GainMod
```

::

> [!Tip:About this Pseudocode] The pseudocode shows the abstract processing model, not literal C++ or HiseScript code. Each type's index page contains pseudocode blocks specific to that module type.

## Common Concepts

All audio modules share these features inherited from the `Processor` base class.

### Module Tree

Modules form a hierarchical tree. A sound generator contains MIDI processors, modulators, and effects as child processors. Container modules (ModulatorSynthChain, SynthGroup) hold multiple sound generators in parallel.

### Processor ID

Every module has a unique string ID used to identify it in the module tree and reference it from scripts. Changing a Processor ID will break existing script references.

> [!Tip:IDE Workflow] Right-click a module's top bar in the HISE IDE to copy its XML to the clipboard or generate a script reference declaration that you can paste into a Script Processor's `onInit` callback.

### Parameters

Modules expose numbered parameters via `setAttribute()` / `getAttribute()`. Each module type defines its own parameter enum. Parameters accept float values and can be automated by the host DAW.

### Bypass

Any module can be bypassed via `setBypassed()`. Effects use a soft bypass with a short crossfade to avoid clicks.

### State Persistence

Module state can be exported as XML (ValueTree) and restored. From scripts, use `Processor.exportState()` and `Processor.restoreState()` with base64-encoded strings to save and restore processor snapshots.

### Scripting Access

Modules are accessed from HiseScript using typed `get*()` methods on the `Synth` namespace. These must be called in `onInit` and stored as `const var` for reuse across callbacks:

```javascript
// Base class references (onInit only)
const var fx      = Synth.getEffect("Delay1");
const var mod     = Synth.getModulator("LFO1");
const var mp      = Synth.getMidiProcessor("Arpeggiator1");
const var child   = Synth.getChildSynth("SubBass");
const var sampler = Synth.getSampler("MainSampler");

// Typed interface references for special data access
const var table   = Synth.getTableProcessor("VeloTable");
const var audio   = Synth.getAudioSampleProcessor("Looper1");
const var slider  = Synth.getSliderPackProcessor("LFOSteps");
const var matrix  = Synth.getRoutingMatrix("MasterChain");
```

Use `setAttribute()` to change parameters, `setIntensity()` / `setBipolar()` for modulator-specific properties (see [Modulators](/v2/reference/audio-modules/modulators/#common-parameters)), and `setBypassed()` to toggle bypass state.

## Categories

An overview of all audio modules grouped by functional category. Modules may appear in more than one category.

---

### Container

Modules that hold and combine other sound generators.

- [Container]($MODULES.SynthChain$): A container for other Sound generators.
- [Synthesiser Group]($MODULES.SynthGroup$): A container for synthesisers that share common modulation, with optional FM synthesis and unison detune/spread.
- [Global Modulator Container]($MODULES.GlobalModulatorContainer$): A container that processes Modulator instances that can be used at different locations.
- [Send Container]($MODULES.SendContainer$): A signal chain tool that receives the signal from a Send FX and applies its own effect chain.
- [Macro Modulation Source]($MODULES.MacroModulationSource$): A container that hosts modulator chains whose output drives the macro control system.

---

### Custom

Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks. Contains sound generators, MIDI processors, effects, and modulators.

Scriptnode modules load interpreted XML networks created in the scriptnode editor via `Engine.createDspNetwork()`. Hardcoded modules load compiled C++ code — either compiled scriptnode networks or custom [C++ DSP nodes]($LANG.cpp-dsp-nodes$) written in `DspNetworks/ThirdParty/*.h`. Both variants share the same hosting infrastructure for parameters, modulation, channels, and complex data.

**Parameters** — Each module type reserves a fixed set of parameters (e.g. sound generators reserve indices 0-3 for Gain, Balance, VoiceLimit, KillFadeTime; envelope modulators reserve indices 0-1 for Monophonic, Retrigger). Network parameters are appended after the reserved indices. Reserved parameter names cannot be reused by the network.

**Modulation** — Extra modulation chain slots bridge HISE modulators into the scriptnode network. Each slot supports two mutually exclusive connection modes: direct parameter modulation or `extra_mod` node pickup. See [Scriptnode Modulation Bridge](/v2/reference/audio-modules/modulators/#scriptnode-modulation-bridge) for details and preprocessor macro configuration.

**Channels** — Scriptnode and hardcoded modules support multichannel configurations up to 16 channels. See [Scriptnode and Hardcoded Module Channels](/v2/reference/audio-modules/sound-generators/#scriptnode-and-hardcoded-module-channels) for details.

**Complex data** — Five complex data types are available: Tables, SliderPacks, AudioFiles, FilterCoefficients, and DisplayBuffers. Each type has its own index sequence (Table[0], Table[1], SliderPack[0], etc.). Data is created on demand when nodes in the network declare their requirements. Complex data is **preserved** across network switches — the data holder is shared across all networks hosted by the module. Access from script via standard processor reference methods (`getTableProcessor()`, `getSliderPackProcessor()`, etc.).

| Module Type | Fixed Params | Param Offset | Voice Context | Built-in Chains |
|---|---|---|---|---|
| Scriptnode Synthesiser / Hardcoded Synthesiser | Gain, Balance, VoiceLimit, KillFadeTime | 4 | polyphonic | Gain, Pitch |
| Script FX / Hardcoded Master FX | (none) | 0 | monophonic | (none) |
| PolyScriptFX / Hardcoded Polyphonic FX | (none) | 0 | polyphonic | (none) |
| Script Envelope / Hardcoded Envelope | Monophonic, Retrigger | 2 | polyphonic (control rate) | (none) |
| Script Time Variant / Hardcoded Time Variant | (none) | 0 | monophonic | (none) |

#### Hardcoded modules

- [Hardcoded Master FX]($MODULES.HardcodedMasterFX$): Runs a compiled C++ DSP network as a master effect, with dynamic parameter and complex data exposure from the network.
- [Hardcoded Polyphonic FX]($MODULES.HardcodedPolyphonicFX$): Runs a compiled C++ DSP network as a polyphonic effect, processing each voice independently with per-voice state.
- [Hardcoded Synthesiser]($MODULES.HardcodedSynth$): Runs a compiled C++ DSP network as a polyphonic sound generator with per-voice processing and full modulator chain support.
- [Hardcoded Envelope Modulator]($MODULES.HardcodedEnvelopeModulator$): Runs a compiled C++ DSP network as a polyphonic envelope modulator with per-voice state and voice management.
- [Hardcoded Time Variant Modulator]($MODULES.HardcodedTimevariantModulator$): Runs a compiled C++ DSP network as a monophonic time-variant modulator with dynamic parameters.

#### Scriptnode modules

- [Script Processor]($MODULES.ScriptProcessor$): The main scripting interface for MIDI processing, UI creation, and plugin control via the HiseScript API.
- [Script FX]($MODULES.ScriptFX$): Processes audio through a scriptnode DSP network as a master effect, with scriptable parameters and complex data routing.
- [Polyphonic Script FX]($MODULES.PolyScriptFX$): Processes each voice independently through a scriptnode DSP network, with per-voice state and polyphonic modulation support.
- [Scriptnode Synthesiser]($MODULES.ScriptSynth$): Generates polyphonic audio from a scriptnode DSP network, with per-voice processing and full modulator chain support.
- [Script Envelope Modulator]($MODULES.ScriptEnvelopeModulator$): Generates a polyphonic envelope signal from a scriptnode DSP network, with per-voice state and voice kill detection.
- [Script Time Variant Modulator]($MODULES.ScriptTimeVariantModulator$): Generates a continuous monophonic modulation signal from a scriptnode DSP network or HiseScript timer callback.
- [Script Voice Start Modulator]($MODULES.ScriptVoiceStartModulator$): Computes a per-voice modulation value at note-on using a HiseScript callback, for custom velocity curves or scripted voice logic.

#### Custom Utility modules

- [Scriptnode Voice Killer]($MODULES.ScriptnodeVoiceKiller$): Monitors a scriptnode envelope's gate signal and terminates voices when the gate closes, required for voice management in scriptnode-based envelopes.
- [Silent Synth]($MODULES.SilentSynth$): A silent sound generator that routes signals through its effect chain without producing audio of its own.

---

### Note Processing

MIDI processors that transform, filter, or react to incoming note events. Contains MIDI processors and modulators.

- [MIDI CC to Note Generator]($MODULES.CC2Note$): Turns a selected MIDI CC into a note trigger, useful for controller-driven drum or round-robin triggering.
- [MIDI Channel Filter]($MODULES.ChannelFilter$): Filters incoming MIDI by channel, with optional MPE start and end channel ranges for MPE setups.
- [MIDI Channel Setter]($MODULES.ChannelSetter$): Rewrites the MIDI channel for all incoming messages, useful for routing or consolidating controllers.
- [Choke Group Processor]($MODULES.ChokeGroupProcessor$): Kills active notes when another choke group processor in the same group receives a note-on, useful for hi-hat and mute group behavior.
- [Notenumber Modulator]($MODULES.KeyNumber$): Creates a modulation value based on the MIDI note number, with optional table mapping for custom response curves.
- [Legato with Retrigger]($MODULES.LegatoWithRetrigger$): Monophonic legato processor that retriggers the previous note after a release, useful for lead lines and expressive legato phrasing.
- [Release Trigger]($MODULES.ReleaseTrigger$): Release trigger generator that replays notes on key-up with velocity scaled by a time-based attenuation curve.
- [Transposer]($MODULES.Transposer$): Transposes incoming MIDI note-on events by a fixed number of semitones for quick key changes or interval shifts.
- [Velocity Modulator]($MODULES.Velocity$): Creates a modulation value from the MIDI velocity of incoming note messages, with optional table mapping and decibel conversion.

---

### Input

Modulators that convert external events like MIDI or MPE into modulation signals.

- [Array Modulator]($MODULES.ArrayModulator$): Creates a modulation signal from a slider pack array indexed by MIDI note number, allowing per-note modulation values.
- [Notenumber Modulator]($MODULES.KeyNumber$): Creates a modulation value based on the MIDI note number, with optional table mapping for custom response curves.
- [MPE Modulator]($MODULES.MPEModulator$): Creates per-voice modulation from MPE pressure, slide, or glide gestures with adjustable smoothing and default values.
- [Midi Controller]($MODULES.MidiController$): Creates a modulation signal from MIDI CC messages with adjustable smoothing and optional table mapping for custom response curves.
- [Pitch Wheel Modulator]($MODULES.PitchWheel$): Creates a monophonic modulation signal from the pitch wheel, with smoothing to reduce stepping artifacts.
- [Velocity Modulator]($MODULES.Velocity$): Creates a modulation value from the MIDI velocity of incoming note messages, with optional table mapping and decibel conversion.

---

### Sequencing

MIDI processors that generate or play back note sequences.

- [Arpeggiator]($MODULES.Arpeggiator$): Configurable arpeggiator with tempo sync, direction, octave range, and per-step sliders for melodic sequencing and rhythm generation.
- [Audio Loop Player]($MODULES.AudioLooper$): A single-file audio player with looping, pitch tracking, tempo sync, and reverse playback.
- [MidiMetronome]($MODULES.MidiMetronome$): A metronome that produces click sounds synchronized to a connected MIDI player's tempo.
- [MIDI Player]($MODULES.MidiPlayer$): Plays MIDI sequences with transport controls, loop regions, and multi-track support for piano roll and step sequencer overlays.

---

### Oscillator

Modules that generate audio or modulation signals from oscillators or synthesis algorithms. Contains sound generators and modulators.

- [LFO Modulator]($MODULES.LFO$): Generates a periodic modulation signal with multiple waveform types, tempo sync, and an optional step sequencer mode.
- [Noise Generator]($MODULES.Noise$): A white noise generator useful for layering, testing signal flow, or as a modulation source.
- [Sine Wave Generator]($MODULES.SineSynth$): A lightweight sine wave generator for FM synthesis, additive synthesis, or adding subtle harmonics to other sounds.
- [Synthesiser Group]($MODULES.SynthGroup$): A container for synthesisers that share common modulation, with optional FM synthesis and unison detune/spread.
- [Waveform Generator]($MODULES.WaveSynth$): A waveform generator based on BLIP synthesis of common synthesiser waveforms.
- [Wavetable Synthesiser]($MODULES.WavetableSynth$): A two-dimensional wavetable synthesiser that morphs between waveforms using a table index and supports audio file resynthesis.

---

### Sample Playback

Modules that play back audio samples.

- [Audio Loop Player]($MODULES.AudioLooper$): A single-file audio player with looping, pitch tracking, tempo sync, and reverse playback.
- [Sampler]($MODULES.StreamingSampler$): A disk-streaming sampler with sample maps, round robin, crossfade groups, and timestretching.

---

### Generator

Modulators that create modulation signals internally, such as envelopes and LFOs. Contains modulators and MIDI processors.

- [AHDSR Envelope]($MODULES.AHDSR$): An AHDSR envelope with adjustable curve shapes, optional downsampling for CPU savings, and per-voice modulation of all time parameters.
- [Arpeggiator]($MODULES.Arpeggiator$): Configurable arpeggiator with tempo sync, direction, octave range, and per-step sliders for melodic sequencing and rhythm generation.
- [Constant]($MODULES.Constant$): Creates a constant modulation signal (1.0) that can be used as a fixed gain offset or modulation source.
- [Flex AHDSR Envelope]($MODULES.FlexAHDSR$): A more complex AHDSR envelope with draggable curves and multiple playback modes.
- [LFO Modulator]($MODULES.LFO$): Generates a periodic modulation signal with multiple waveform types, tempo sync, and an optional step sequencer mode.
- [MIDI Player]($MODULES.MidiPlayer$): Plays MIDI sequences with transport controls, loop regions, and multi-track support for piano roll and step sequencer overlays.
- [Random Modulator]($MODULES.Random$): Generates a random value at each voice start, with optional table mapping for custom probability distributions.
- [Simple Envelope]($MODULES.SimpleEnvelope$): A lightweight two-stage envelope with attack and release, supporting linear or exponential curves.
- [Table Envelope]($MODULES.TableEnvelope$): An envelope with fully customizable attack and release shapes drawn as lookup tables.

---

### Filter

Effects that shape the frequency spectrum of the audio signal.

- [Parametriq EQ]($MODULES.CurveEq$): A parametric equalizer with unlimited filter bands and an FFT spectrum display for visual feedback.
- [Harmonic Filter]($MODULES.HarmonicFilter$): Polyphonic peak filters tuned to the root frequency and harmonics of each voice, with crossfadeable A/B slider pack configurations.
- [Harmonic Filter Monophonic]($MODULES.HarmonicFilterMono$): Monophonic peak filters tuned to the root frequency and harmonics of the last played note, with crossfadeable A/B configurations.
- [Filter]($MODULES.PolyphonicFilter$): Applies monophonic or polyphonic filtering with modulatable frequency, gain, and resonance, supporting multiple filter types.

---

### Dynamics

Effects that shape the amplitude or add distortion and saturation.

- [Dynamics]($MODULES.Dynamics$): A dynamics processor combining gate, compressor, and limiter based on chunkware's SimpleCompressor algorithms.
- [Polyshape FX]($MODULES.PolyshapeFX$): A polyphonic waveshaper with multiple shaping modes, table-based curves, and optional oversampling.
- [Saturator]($MODULES.Saturator$): Applies waveshaping saturation with pre/post gain controls and wet/dry mix.
- [Shape FX]($MODULES.ShapeFX$): Waveshaper effect with selectable shaping modes, bias, filters, and oversampling, suitable for distortion and tone shaping with optional autogain.

---

### Delay

Effects based on delayed signal copies, including chorus and phaser.

- [Chorus]($MODULES.Chorus$): Stereo chorus effect with modulated delay lines for thickening and movement.
- [Delay]($MODULES.Delay$): Stereo delay with independent left and right times, feedback, and filtering, with optional tempo sync for rhythmic echo effects.
- [Phase FX]($MODULES.PhaseFX$): Phaser effect using modulated allpass filters for sweeping frequency notches.

---

### Reverb

Effects that simulate room acoustics and spatial reflections.

- [Convolution Reverb]($MODULES.Convolution$): Zero-latency convolution reverb with adjustable dry/wet levels, predelay, damping, and high-cut filtering for shaping impulse responses.
- [Simple Reverb]($MODULES.SimpleReverb$): Algorithmic reverb based on Freeverb with controls for room size, damping, and stereo width.

---

### Routing

Modules that forward, distribute, or proxy signals or events across the module tree. Contains sound generators, MIDI processors, effects, and modulators.

- [MIDI CC to Note Generator]($MODULES.CC2Note$): Turns a selected MIDI CC into a note trigger, useful for controller-driven drum or round-robin triggering.
- [CC Swapper]($MODULES.CCSwapper$): Swaps two MIDI CC numbers to remap controller data without changing the source device or automation.
- [MIDI Channel Filter]($MODULES.ChannelFilter$): Filters incoming MIDI by channel, with optional MPE start and end channel ranges for MPE setups.
- [MIDI Channel Setter]($MODULES.ChannelSetter$): Rewrites the MIDI channel for all incoming messages, useful for routing or consolidating controllers.
- [EventData Envelope]($MODULES.EventDataEnvelope$): An envelope modulator for time-varying event data slots, with smoothing for continuous modulation changes.
- [Event Data Modulator]($MODULES.EventDataModulator$): Creates a modulation value based on event data written through the global routing manager, allowing external control data to modulate voices.
- [Global Envelope Modulator]($MODULES.GlobalEnvelopeModulator$): Connects to a global EnvelopeModulator in a GlobalModulatorContainer, allowing envelope modulation to be shared across multiple targets.
- [Global Modulator Container]($MODULES.GlobalModulatorContainer$): A container that processes Modulator instances that can be used at different locations.
- [Global Static Time Variant Modulator]($MODULES.GlobalStaticTimeVariantModulator$): Captures the current value of a global TimeVariantModulator at voice start, creating a constant per-voice modulation based on the LFO/envelope state at note-on.
- [Global Time Variant Modulator]($MODULES.GlobalTimeVariantModulator$): Shares a global TimeVariantModulator signal across multiple targets, allowing real-time continuous modulation from a single source.
- [Global Voice Start Modulator]($MODULES.GlobalVoiceStartModulator$): Connects to a global VoiceStartModulator in a GlobalModulatorContainer, allowing voice-start modulation to be shared across multiple targets.
- [Macro Modulation Source]($MODULES.MacroModulationSource$): A container that hosts modulator chains whose output drives the macro control system.
- [Macro Modulator]($MODULES.MacroModulator$): A modulator controlled by a macro controller slot, allowing real-time automation and MIDI learn functionality.
- [Matrix Modulator]($MODULES.MatrixModulator$): Combines multiple global modulators into a single modulation source with a base value and smoothed output.
- [MidiMuter]($MODULES.MidiMuter$): Mutes incoming note-on events while allowing other MIDI messages, with optional stuck-note protection.
- [Routing Matrix]($MODULES.RouteFX$): Routing matrix for duplicating and distributing audio across channels, useful for building aux-style signal paths and complex channel layouts.
- [Send Container]($MODULES.SendContainer$): A signal chain tool that receives the signal from a Send FX and applies its own effect chain.
- [Send Effect]($MODULES.SendFX$): Routes audio to a send container with adjustable gain, channel offset, and optional smoothing for consistent send automation.
- [Effect Slot]($MODULES.SlotFX$): A placeholder for another effect that can be swapped dynamically.

---

### Utility

Modules for analysis, placeholders, or structural purposes without audio processing.

- [Analyser]($MODULES.Analyser$): Provides audio visualization tools including goniometer, oscilloscope, and spectrum analyzer.
- [Empty]($MODULES.EmptyFX$): A placeholder effect that passes audio through unchanged, useful for routing or as a template.
- [MidiMetronome]($MODULES.MidiMetronome$): A metronome that produces click sounds synchronized to a connected MIDI player's tempo.
- [Noise Grain Player]($MODULES.NoiseGrainPlayer$): A polyphonic granular noise player that blends an audio file with white noise at a configurable grain size.
- [Simple Gain]($MODULES.SimpleGain$): Utility gain processor with optional delay, stereo width, and balance control, useful for level automation, simple timing offsets, and mid-side shaping.
- [Effect Slot]($MODULES.SlotFX$): A placeholder for another effect that can be swapped dynamically.

---

### Mixing

Effects that control volume, stereo width, or stereo balance. Contains effects and MIDI processors.

- [Parametriq EQ]($MODULES.CurveEq$): A parametric equalizer with unlimited filter bands and an FFT spectrum display for visual feedback.
- [Dynamics]($MODULES.Dynamics$): A dynamics processor combining gate, compressor, and limiter based on chunkware's SimpleCompressor algorithms.
- [MidiMuter]($MODULES.MidiMuter$): Mutes incoming note-on events while allowing other MIDI messages, with optional stuck-note protection.
- [Send Effect]($MODULES.SendFX$): Routes audio to a send container with adjustable gain, channel offset, and optional smoothing for consistent send automation.
- [Simple Gain]($MODULES.SimpleGain$): Utility gain processor with optional delay, stereo width, and balance control, useful for level automation, simple timing offsets, and mid-side shaping.
- [Stereo FX]($MODULES.StereoFX$): Polyphonic stereo panner with width control and modulatable pan position.
